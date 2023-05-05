/************************************************\
 ------------------------------------------------
                DiDaC (UCI-compatible)
 ------------------------------------------------
\************************************************/

// REQ HEADERS
#include <stdio.h>
#include <string.h>
#include "sys/time.h"
#include "sys/select.h"
#include "string.h"

// Copied from Wukong.c by Maksim Korzh


// FEN START POSITION
#define startingFEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "

/************************************************\
 ------------------------------------------------
                BOARD CONSTRUCTION
 ------------------------------------------------
\************************************************/

// ENCODING PIECES (e implies empty)
enum pieces {e, P, N, B, R, Q, K, p, n, b, r, q, k, o};

// ENCODING SQUARES (Note: rightmost squares are classified as empty)
enum squares {
    a8 = 0,   b8, c8, d8, e8, f8, g8, h8,
    a7 = 16,  b7, c7, d7, e7, f7, g7, h7,
    a6 = 32,  b6, c6, d6, e6, f6, g6, h6,
    a5 = 48,  b5, c5, d5, e5, f5, g5, h5,
    a4 = 64,  b4, c4, d4, e4, f4, g4, h4,
    a3 = 80,  b3, c3, d3, e3, f3, g3, h3,
    a2 = 96,  b2, c2, d2, e2, f2, g2, h2,
    a1 = 112, b1, c1, d1, e1, f1, g1, h1, no_sq
};

// FLAG FOR CAPTURES
enum capture_flags {all_moves, only_captures};

// SIDES
enum sides { white, black };

// UNICODE PIECES
char *unicode_pieces[] = {".", "♙", "♘", "♗", "♖", "♕", "♔", "♟︎", "♞", "♝", "♜", "♛", "♚"};

// ENCODE ASCII PIECES
int char_pieces[] = {
    ['P'] = P,
    ['N'] = N,
    ['B'] = B,
    ['R'] = R,
    ['Q'] = Q,
    ['K'] = K,
    ['p'] = p,
    ['n'] = n,
    ['b'] = b,
    ['r'] = r,
    ['q'] = q,
    ['k'] = k,
};

// DECODE PROMOTED PIECES
int promoted_pieces[] = {
    [Q] = 'q',
    [R] = 'r',
    [B] = 'b',
    [N] = 'n',
    [q] = 'q',
    [r] = 'r',
    [b] = 'b',
    [n] = 'n',
};

//***BEGIN DERIVED CODE (changes to material score based on online research)***

// MATERIAL SCORE (using values from Tomasz Michniewski's Simplified Evaluation from https://www.chessprogramming.org/Simplified_Evaluation_Function)

int material_score[13] = {
      0,      // empty square score
    100,      // white pawn score
    320,      // white knight scrore
    330,      // white bishop score
    500,      // white rook score
    900,      // white queen score
  20000,      // white king score
   -100,      // black pawn score
   -320,      // black knight scrore
   -330,      // black bishop score
   -500,      // black rook score
    -900,      // black queen score
 -20000,      // black king score
    
};

//***END DERIVED CODE***

// Copied from Wukong.c by Maksim Korzh

/************************************************\
 ------------------------------------------------
    CASTLING BINARY REPRESENTATION 
 ------------------------------------------------
\************************************************/

//  bin  dec
// 0001    1  WHITE KING CAN CASTLE KING SIDE
// 0010    2  WHITE KING CAN CASTLE QUEEN SIDE
// 0100    4  BLACK KING CAN CASTLE KING SIDE
// 1000    8  BLACK KING CAN CASTLE QUEEN SIDE
//
// THEREFORE,
// 1111       BOTH SIDES CAN CASTLE BOTH DIRECTIONS
// 1001       BLACK KING => QUEEN SIDE

// CASTLING RIGHTS
enum castling { KC = 1, QC = 2, kc = 4, qc = 8 };

/*
                             castle   move     in      in
                              right    map     binary  decimal

        WHITE KING  MOVED:     1111 & 1100  =  1100    12
  WHITE KING'S ROOK MOVED:     1111 & 1110  =  1110    14
 WHITE QUEEN'S ROOK MOVED:     1111 & 1101  =  1101    13
     
         BLACK KING MOVED:     1111 & 0011  =  1011    3
  BLACK KING'S ROOK MOVED:     1111 & 1011  =  1011    11
 BLACK QUEEN'S ROOK MOVED:     1111 & 0111  =  0111    7

*/

int castling_rights[128] = {
     7, 15, 15, 15,  3, 15, 15, 11,  o, o, o, o, o, o, o, o,
    15, 15, 15, 15, 15, 15, 15, 15,  o, o, o, o, o, o, o, o,
    15, 15, 15, 15, 15, 15, 15, 15,  o, o, o, o, o, o, o, o,
    15, 15, 15, 15, 15, 15, 15, 15,  o, o, o, o, o, o, o, o,
    15, 15, 15, 15, 15, 15, 15, 15,  o, o, o, o, o, o, o, o,
    15, 15, 15, 15, 15, 15, 15, 15,  o, o, o, o, o, o, o, o,
    15, 15, 15, 15, 15, 15, 15, 15,  o, o, o, o, o, o, o, o,
    13, 15, 15, 15, 12, 15, 15, 14,  o, o, o, o, o, o, o, o
};


//***BEGIN DERIVED CODE (changes to original PSTs based on Tomasz Michniewski's work)***

/************************************************\
 ------------------------------------------------
        POSITIONAL SCORES (inspired from Tomasz Michniewski : https://www.chessprogramming.org/Simplified_Evaluation_Function)
 ------------------------------------------------
\************************************************/

// PAWN POSITIONAL SCORE
const int pawn_score[128] = 
{

     0,  0,  0,  0,  0,  0,  0,  0,  o, o, o, o, o, o, o, o,
    50, 50, 50, 50, 50, 50, 50, 50,  o, o, o, o, o, o, o, o,
    10, 10, 20, 30, 30, 20, 10, 10,  o, o, o, o, o, o, o, o,
    5,  5, 10, 25, 25, 10,  5,  5,   o, o, o, o, o, o, o, o,
    0,  0,  0, 20, 20,  0,  0,  0,   o, o, o, o, o, o, o, o,
    5, -5,-10,  0,  0,-10, -5,  5,   o, o, o, o, o, o, o, o,
    5, 10, 10,-20,-20, 10, 10,  5,   o, o, o, o, o, o, o, o,
    0,  0,  0,  0,  0,  0,  0,  0,   o, o, o, o, o, o, o, o,
};

// knight positional score
const int knight_score[128] = 
{
    -50,-40,-30,-30,-30,-30,-40,-50, o, o, o, o, o, o, o, o,
    -40,-20,  0,  0,  0,  0,-20,-40, o, o, o, o, o, o, o, o,
    -30,  0, 10, 15, 15, 10,  0,-30, o, o, o, o, o, o, o, o,
    -30,  5, 15, 20, 20, 15,  5,-30, o, o, o, o, o, o, o, o,
    -30,  0, 15, 20, 20, 15,  0,-30, o, o, o, o, o, o, o, o,
    -30,  5, 10, 15, 15, 10,  5,-30, o, o, o, o, o, o, o, o,
    -40,-20,  0,  5,  5,  0,-20,-40, o, o, o, o, o, o, o, o,
    -50,-40,-30,-30,-30,-30,-40,-50, o, o, o, o, o, o, o, o,
};

// bishop positional score
const int bishop_score[128] = 


{
    -20,-10,-10,-10,-10,-10,-10,-20,  o, o, o, o, o, o, o, o,
    -10,  0,  0,  0,  0,  0,  0,-10,  o, o, o, o, o, o, o, o,
    -10,  0,  5, 10, 10,  5,  0,-10,  o, o, o, o, o, o, o, o,
    -10,  5,  5, 10, 10,  5,  5,-10,  o, o, o, o, o, o, o, o,
    -10,  0, 10, 10, 10, 10,  0,-10,  o, o, o, o, o, o, o, o,
    -10, 10, 10, 10, 10, 10, 10,-10,  o, o, o, o, o, o, o, o,
    -10,  5,  0,  0,  0,  0,  5,-10,  o, o, o, o, o, o, o, o,
    -20,-10,-10,-10,-10,-10,-10,-20,  o, o, o, o, o, o, o, o,
};

// rook positional score
const int rook_score[128] =
{  0,  0,  0,  0,  0,  0,  0,  0,  o, o, o, o, o, o, o, o,
  5, 10, 10, 10, 10, 10, 10,  5,  o, o, o, o, o, o, o, o,
 -5,  0,  0,  0,  0,  0,  0, -5,  o, o, o, o, o, o, o, o,
 -5,  0,  0,  0,  0,  0,  0, -5,  o, o, o, o, o, o, o, o,
 -5,  0,  0,  0,  0,  0,  0, -5,  o, o, o, o, o, o, o, o,
 -5,  0,  0,  0,  0,  0,  0, -5,  o, o, o, o, o, o, o, o,
 -5,  0,  0,  0,  0,  0,  0, -5,  o, o, o, o, o, o, o, o,
  0,  0,  0,  5,  5,  0,  0,  0,  o, o, o, o, o, o, o, o,

};

// king positional score
const int king_score[128] = 
{
     0,   0,   0,   0,   0,   0,   0,   0,    o, o, o, o, o, o, o, o,
     0,   0,   5,   5,   5,   5,   0,   0,    o, o, o, o, o, o, o, o,
     0,   5,   5,  10,  10,   5,   5,   0,    o, o, o, o, o, o, o, o,
     0,   5,  10,  20,  20,  10,   5,   0,    o, o, o, o, o, o, o, o,
     0,   5,  10,  20,  20,  10,   5,   0,    o, o, o, o, o, o, o, o,
     0,   0,   5,  10,  10,   5,   0,   0,    o, o, o, o, o, o, o, o,
     0,   5,   5,  -5,  -5,   0,   5,   0,    o, o, o, o, o, o, o, o,
     0,   0,   5,   0, -15,   0,  10,   0,    o, o, o, o, o, o, o, o

};

//***END DERIVED CODE***
// Copied from Wukong.c by Maksim Korzh

//

// mirror positional score tables for opposite side
const int mirror_score[128] =
{
	a1, b1, c1, d1, e1, f1, g1, h1,    o, o, o, o, o, o, o, o,
	a2, b2, c2, d2, e2, f2, g2, h2,    o, o, o, o, o, o, o, o,
	a3, b3, c3, d3, e3, f3, g3, h3,    o, o, o, o, o, o, o, o,
	a4, b4, c4, d4, e4, f4, g4, h4,    o, o, o, o, o, o, o, o,
	a5, b5, c5, d5, e5, f5, g5, h5,    o, o, o, o, o, o, o, o,
	a6, b6, c6, d6, e6, f6, g6, h6,    o, o, o, o, o, o, o, o,
	a7, b7, c7, d7, e7, f7, g7, h7,    o, o, o, o, o, o, o, o,
	a8, b8, c8, d8, e8, f8, g8, h8,    o, o, o, o, o, o, o, o
};


// chess board representation
int board[128] = {
    r, n, b, q, k, b, n, r,  o, o, o, o, o, o, o, o,
    p, p, p, p, p, p, p, p,  o, o, o, o, o, o, o, o,
    e, e, e, e, e, e, e, e,  o, o, o, o, o, o, o, o,
    e, e, e, e, e, e, e, e,  o, o, o, o, o, o, o, o,
    e, e, e, e, e, e, e, e,  o, o, o, o, o, o, o, o,
    e, e, e, e, e, e, e, e,  o, o, o, o, o, o, o, o,
    P, P, P, P, P, P, P, P,  o, o, o, o, o, o, o, o,
    R, N, B, Q, K, B, N, R,  o, o, o, o, o, o, o, o
};


/************************************************\
 ------------------------------------------------
    BOARD INITIALIZATION
 ------------------------------------------------
\************************************************/
// SIDE TO MOVE
int side = white;

// ENPASSANT SQUARE
int enpassant = no_sq;

// CASTLING RIGHTS (BOTH KINGS CAN CASTLE TO BOTH SIDES)
int castle = 15;

// KINGS'S SQUARES
int king_square[2] = {e1, e8};

/*
    Move formatting
    
    0000 0000 0000 0000 0111 1111       source square
    0000 0000 0011 1111 1000 0000       target square
    0000 0011 1100 0000 0000 0000       promoted piece
    0000 0100 0000 0000 0000 0000       capture flag
    0000 1000 0000 0000 0000 0000       double pawn flag
    0001 0000 0000 0000 0000 0000       enpassant flag
    0010 0000 0000 0000 0000 0000       castling

*/

// ENCODE MOVE
#define encode_move(source, target, piece, capture, pawn, enpassant, castling) \
(                          \
    (source) |             \
    (target << 7) |        \
    (piece << 14) |        \
    (capture << 18) |      \
    (pawn << 19) |         \
    (enpassant << 20) |    \
    (castling << 21)       \
)

// DECODE SOURCE SQUARE
#define get_move_source(move) (move & 0x7f)

// DECODE TARGET SQUARE
#define get_move_target(move) ((move >> 7) & 0x7f)

// DECODE PROMOTED PIECE
#define get_move_piece(move) ((move >> 14) & 0xf)

// DECODE CAPTURE FLAG
#define get_move_capture(move) ((move >> 18) & 0x1)

// DECODE DOUBLE PAWN PUSH (i.e. from second row) FLAG
#define get_move_pawn(move) ((move >> 19) & 0x1)

// DECODE ENPASSANT FLAG
#define get_move_enpassant(move) ((move >> 20) & 0x1)

// DECODE CASTLING SQUARE FLAG
#define get_move_castling(move) ((move >> 21) & 0x1)

// BOARD SQUARE TO COORDS
char *square_to_coords[] = {
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8", "i8", "j8", "k8", "l8", "m8", "n8", "o8", "p8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", "i7", "j7", "k7", "l7", "m7", "n7", "o7", "p7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", "i6", "j6", "k6", "l6", "m6", "n6", "o6", "p6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", "i5", "j5", "k5", "l5", "m5", "n5", "o5", "p5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", "i4", "j4", "k4", "l4", "m4", "n4", "o4", "p4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", "i3", "j3", "k3", "l3", "m3", "n3", "o3", "p3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2", "i2", "j2", "k2", "l2", "m2", "n2", "o2", "p2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", "i1", "j1", "k1", "l1", "m1", "n1", "o1", "p1"
};
/***********************************************\

                 BOARD FUNCTIONS 

\***********************************************/

// PRINT BOARD
void print_board()
{
    // PRINT NEW LINE
    printf("\n");

    // LOOP OVER BOARD ROWS
    for (int row = 0; row < 8; row++)
    {
        // LOOP OVER BOARD COLUMNS
        for (int columns = 0; columns < 16; columns++)
        {
            // INIT SQUARE
            int square = row * 16 + columns;
            
            // PRINT ROWS
            if (columns == 0)
                printf(" %d  ", 8 - row);
            
            // IF SQUARE IS ON BOARD
            if (!(square & 0x88))
                    // print UNICODE pieces on linux
                    printf("%s ", unicode_pieces[board[square]]);
        }
        
        // PRINT NEW LINE EVERY TIME NEW ROW IS ENCOUNTERED
        printf("\n");
    }
    
    // PRINT COLUMNS
    printf("\n    a b c d e f g h\n\n");
   
}

// PRINT MOVE LIST
void print_move_list(moves *move_list)
{
    // TABLE HEADER
    printf("\n    Move     Capture  Double   Enpass   Castling\n\n");

    // LOOP OVER MOVES
    for (int index = 0; index < move_list->count; index++)
    {
        int move = move_list->moves[index];
        printf("    %s%s", square_to_coords[get_move_source(move)], square_to_coords[get_move_target(move)]);
        printf("%c    ", get_move_piece(move) ? promoted_pieces[get_move_piece(move)] : ' ');
        printf("%d        %d        %d        %d\n", get_move_capture(move), get_move_pawn(move), get_move_enpassant(move), get_move_castling(move));
    }
    printf("\n    Total moves: %d\n\n", move_list->count);
}

// RESET BOARD
void reset_board()
{
    // LOOP OVER BOARD ROWS
    for (int rank = 0; rank < 8; rank++)
    {
        // LOOP OVER BOARD COLUMNS
        for (int file = 0; file < 16; file++)
        {
            // INIT SQUARE
            int square = rank * 16 + file;
        
            // IF SQUARE IS ON BOARD
            if (!(square & 0x88))
                // RESET CURRENT BOARD SQUARE
                board[square] = e;
        }
    }
    
    // RESET STATS
    side = -1;
    castle = 0;
    enpassant = no_sq;
}

// parse FEN (Inspired from maksimKorzh/wukongJS Lines 1800 onwarcs)
void parse_fen(char *fenString)
{
    // RESET BOARD
    reset_board();
    
    // LOOP OVER BOARD RANKS
    for (int rank = 0; rank < 8; rank++)
    {
        // LOOP OVER BOARD FILES
        for (int file = 0; file < 16; file++)
        {
            // INIT SQUARE
            int square = rank * 16 + file;
        
            // IF SQUARE IS ON BOARD
            if (!(square & 0x88))
            {
                // MATCH PIECES
                if ((*fenString >= 'a' && *fenString <= 'z') || (*fenString >= 'A' && *fenString <= 'Z'))
                {
                    // SET UP KINGS' SQUARES
                    if (*fenString == 'K')
                        king_square[white] = square;
                    
                    else if (*fenString == 'k')
                        king_square[black] = square;
                    
                    // SET THE PIECE ON BOARD
                    board[square] = char_pieces[*fenString];
                    
                    // INCREMENT FEN POINTER
                    *fenString++;
                }
                
                // MATCH EMPTY SQUARES
                if (*fenString >= '0' && *fenString <= '9')
                {
                    // CALCULATE OFFSET
                    int offset = *fenString - '0';
                    
                    // DECREMENT FILE ON EMPTY SQUARES
                    if (!(board[square]))
                        file--;
                    
                    // SKIP EMPTY SQUARES
                    file += offset;
                    
                    // increment FEN pointer
                    *fenString++;
                }
                
                // match end of rank
                if (*fenString == '/')
                    // increment FEN pointer
                    *fenString++;
                
            }
        }
    }
    
    // go to side parsing
    *fenString++;
    
    // parse side to move
    side = (*fenString == 'w') ? white : black;
    
    // go to castling rights parsing
    fenString += 2;
    
    // parse castling rights
    while (*fenString != ' ')
    {
        switch(*fenString)
        {
            case 'K': castle |= KC; break;
            case 'Q': castle |= QC; break;
            case 'k': castle |= kc; break;
            case 'q': castle |= qc; break;
            case '-': break;
        }
        
        // increment pointer
        *fenString++;
    }
    
    // got to empassant square
    *fenString++;
    
    // parse empassant square
    if (*fenString != '-')
    {
        // parse enpassant square's file & rank
        int file = fenString[0] - 'a';
        int rank = 8 - (fenString[1] - '0');
        
        // set up enpassant square
        enpassant = rank * 16 + file;
    }
    
    else
        enpassant = no_sq;   
}

  /****************************\
   ============================
   
           MOVE OFFSETS (necessary to find next possible movements)


   ============================              
  \****************************/

// PIECE MOVE OFFSETS
int knight_offsets[8] = {33, 31, 18, 14, -33, -31, -18, -14};
int bishop_offsets[4] = {15, 17, -15, -17};
int rook_offsets[4] = {16, -16, 1, -1};
int king_offsets[8] = {16, -16, 1, -1, 15, 17, -15, -17};

// Note: Queen offsets will be a combination of the rook and bishop offsets.

// MOVE LIST STRUCTURE
typedef struct {
    // MOVE LIST
    int moves[256];
    
    // MOVE COUNT
    int count;
} moves;



/***********************************************\

             MOVE GENERATOR FUNCTIONS 

\***********************************************/

// IS SQUARE ATTACKED (Also looked at Attack.c from Vice : https://github.com/bluefeversoft/vice/blob/main/Vice11/src/attack.c))
static inline int is_square_attacked(int square, int side) 
{
    // PAWN ATTACKS
    if (!side)
    {
        // IF TARGET SQUARE IS ON BOARD AND IS WHITE PAWN
        if (!((square + 17) & 0x88) && (board[square + 17] == P))
            return 1;
        
        // IF TARGET SQUARE IS ON BOARD AND IS WHITE PAWN
        if (!((square + 15) & 0x88) && (board[square + 15] == P))
            return 1;
    }
    
    else
    {
        // IF TARGET SQUARE IS ON BOARD AND IS BLACK PAWN
        if (!((square - 17) & 0x88) && (board[square - 17] == p))
            return 1;
        
        // IF TARGET SQUARE IS ON BOARD AND IS BLACK PAWN
        if (!((square - 15) & 0x88) && (board[square - 15] == p))
            return 1;
    }
    
    // SAME FOR KNIGHTS, USING KNIGHT_OFFSETS
    for (int index = 0; index < 8; index++)
    {
        int target_square = square + knight_offsets[index];
        
        // LOOKUP TARGET PIECE
        int target_piece = board[target_square];
        
        // IF TARGET SQUARE IS ON BOARD
        if (!(target_square & 0x88))
        {
            if (!side ? target_piece == N : target_piece == n)
                return 1;
        } 
    }
    
    // SAME FOR KINGS, USING KNIGHT_OFFSETS
    for (int index = 0; index < 8; index++)
    {
        int target_square = square + king_offsets[index];
        
        // LOOKUP TARGET PIECE
        int target_piece = board[target_square];
        
        // IF TARGET SQUARE IS ON BOARD
        if (!(target_square & 0x88))
        {
            // iF TARGET PIECE IS EITHER WHITE OR BLACK KING
            if (!side ? target_piece == K : target_piece == k)
                return 1;
        } 
    }
    
    // BISHOP & QUEEN ATTACKS
    for (int index = 0; index < 4; index++)
    {
        // INIT TARGET SQUARE
        int target_square = square + bishop_offsets[index];
        
        // LOOP OVER ATTACK RAY
        while (!(target_square & 0x88))
        {
            // TARGET PIECE
            int target_piece = board[target_square];
            
            // IF TARGET PIECE IS EITHER WHITE OR BLACK BISHOP OR QUEEN
            if (!side ? (target_piece == B || target_piece == Q) : (target_piece == b || target_piece == q))
                return 1;

            // BREAK IF HIT A PIECE
            if (target_piece)
                break;
        
            // INCREMENT TARGET SQUARE BY MOVE OFFSET
            target_square += bishop_offsets[index];
        }
    }
    
    // ROOK & QUEEN ATTACKS
    for (int index = 0; index < 4; index++)
    {
        // INIT TARGET SQUARE
        int target_square = square + rook_offsets[index];
        
        // LOOP OVER ATTACK RAY
        while (!(target_square & 0x88))
        {
            // TARGET PIECE
            int target_piece = board[target_square];
            
            // IF TARGET PIECE IS EITHER WHITE OR BLACK BISHOP OR QUEEN
            if (!side ? (target_piece == R || target_piece == Q) : (target_piece == r || target_piece == q))
                return 1;

            // BREAK IF HIT A PIECE
            if (target_piece)
                break;
        
            // INCREMENT TARGET SQUARE BY MOVE OFFSET
            target_square += rook_offsets[index];
        }
    }
    
    return 0;
}

// PRINT ATTACK MAP
void print_attacked_squares(int side)
{
    // print new line
    printf("\n");
    printf("    Attacking side: %s\n\n", !side ? "white" : "black");

    // LOOP OVER BOARD RANKS
    for (int rank = 0; rank < 8; rank++)
    {
        // LOOP OVER BOARD FILES
        for (int file = 0; file < 16; file++)
        {
            // INIT SQUARE
            int square = rank * 16 + file;
            
            // PRINT RANKS
            if (file == 0)
                printf(" %d  ", 8 - rank);
            
            // IF SQUARE IS ON BOARD
            if (!(square & 0x88))
                printf("%c ", is_square_attacked(square, side) ? 'x' : '.');
                
        }
        
        // PRINT NEW LINE EVERY TIME NEW RANK IS ENCOUNTERED
        printf("\n");
    }
    
    printf("\n    a b c d e f g h\n\n");
}

// POPULATE MOVE LIST
static inline void add_move(moves *move_list, int move)
{
    // PUSH MOVE INTO THE MOVE LIST
    move_list->moves[move_list->count] = move;
    
    // INCREMENT MOVE COUNT
    move_list->count++;
}


// MOVE GENERATOR (Inspired from wukongJS (lines 440 onwards) https://github.com/maksimKorzh/wukongJS/blob/main/wukong.js)
static inline void generate_moves(moves *move_list)
{
    // RESET MOVE COUNT
    move_list->count = 0;

    // LOOP OVER ALL BOARD SQUARES
    for (int square = 0; square < 128; square++)
    {
        // CHECK IF THE SQUARE IS ON BOARD
        if (!(square & 0x88))
        {
            // WHITE PAWN AND CASTLING MOVES
            if (!side)
            {
                // WHITE PAWN MOVES
                if (board[square] == P)
                {
                    // init target square
                    int to_square = square - 16;
                    
                    // quite white pawn moves (check if target square is on board)
                    if (!(to_square & 0x88) && !board[to_square])
                    {   
                        // pawn promotions
                        if (square >= a7 && square <= h7)
                        {
                            add_move(move_list, encode_move(square, to_square, Q, 0, 0, 0, 0));
                            add_move(move_list, encode_move(square, to_square, R, 0, 0, 0, 0));
                            add_move(move_list, encode_move(square, to_square, B, 0, 0, 0, 0));
                            add_move(move_list, encode_move(square, to_square, N, 0, 0, 0, 0));                            
                        }
                        
                        else
                        {
                            // one square ahead pawn move
                            add_move(move_list, encode_move(square, to_square, 0, 0, 0, 0, 0));
                            
                            // two squares ahead pawn move
                            if ((square >= a2 && square <= h2) && !board[square - 32])
                                add_move(move_list, encode_move(square, square - 32, 0, 0, 1, 0, 0));
                        }
                    }
                    
                    // white pawn capture moves
                    for (int index = 0; index < 4; index++)
                    {
                        // init pawn offset
                        int pawn_offset = bishop_offsets[index];
                        
                        // white pawn offsets
                        if (pawn_offset < 0)
                        {
                            // init target square
                            int to_square = square + pawn_offset;
                            
                            // check if target square is on board
                            if (!(to_square & 0x88))
                            {
                                // capture pawn promotion
                                if (
                                     (square >= a7 && square <= h7) &&
                                     (board[to_square] >= 7 && board[to_square] <= 12)
                                   )
                                {
                                    add_move(move_list, encode_move(square, to_square, Q, 1, 0, 0, 0));
                                    add_move(move_list, encode_move(square, to_square, R, 1, 0, 0, 0));
                                    add_move(move_list, encode_move(square, to_square, B, 1, 0, 0, 0));
                                    add_move(move_list, encode_move(square, to_square, N, 1, 0, 0, 0));
                                }
                                
                                else
                                {
                                    // casual capture
                                    if (board[to_square] >= 7 && board[to_square] <= 12)
                                        add_move(move_list, encode_move(square, to_square, 0, 1, 0, 0, 0));
                                    
                                    // enpassant capture
                                    if (to_square == enpassant)
                                        add_move(move_list, encode_move(square, to_square, 0, 1, 0, 1, 0));
                                }
                            }
                        }
                    }
                }
                
                // white king castling
                if (board[square] == K)
                {
                    // if king side castling is available
                    if (castle & KC)
                    {
                        // make sure there are empty squares between king & rook
                        if (!board[f1] && !board[g1])
                        {
                            // make sure king & next square are not under attack
                            if (!is_square_attacked(e1, black) && !is_square_attacked(f1, black))
                                add_move(move_list, encode_move(e1, g1, 0, 0, 0, 0, 1));
                        }
                    }
                    
                    // if queen side castling is available
                    if (castle & QC)
                    {
                        // make sure there are empty squares between king & rook
                        if (!board[d1] && !board[b1] && !board[c1])
                        {
                            // make sure king & next square are not under attack
                            if (!is_square_attacked(e1, black) && !is_square_attacked(d1, black))
                                add_move(move_list, encode_move(e1, c1, 0, 0, 0, 0, 1));
                        }
                    }
                }
            }
            
            // black pawn and castling moves
            else
            {
                // black pawn moves
                if (board[square] == p)
                {
                    // init target square
                    int to_square = square + 16;
                    
                    // quite black pawn moves (check if target square is on board)
                    if (!(to_square & 0x88) && !board[to_square])
                    {   
                        // pawn promotions
                        if (square >= a2 && square <= h2)
                        {
                            add_move(move_list, encode_move(square, to_square, q, 0, 0, 0, 0));
                            add_move(move_list, encode_move(square, to_square, r, 0, 0, 0, 0));
                            add_move(move_list, encode_move(square, to_square, b, 0, 0, 0, 0));
                            add_move(move_list, encode_move(square, to_square, n, 0, 0, 0, 0));
                        }
                        
                        else
                        {
                            // one square ahead pawn move
                            add_move(move_list, encode_move(square, to_square, 0, 0, 0, 0, 0));
                            
                            // two squares ahead pawn move
                            if ((square >= a7 && square <= h7) && !board[square + 32])
                                add_move(move_list, encode_move(square, square + 32, 0, 0, 1, 0, 0));
                        }
                    }
                    
                    // black pawn capture moves
                    for (int index = 0; index < 4; index++)
                    {
                        // init pawn offset
                        int pawn_offset = bishop_offsets[index];
                        
                        // white pawn offsets
                        if (pawn_offset > 0)
                        {
                            // init target square
                            int to_square = square + pawn_offset;
                            
                            // check if target square is on board
                            if (!(to_square & 0x88))
                            {
                                // capture pawn promotion
                                if (
                                     (square >= a2 && square <= h2) &&
                                     (board[to_square] >= 1 && board[to_square] <= 6)
                                   )
                                {
                                    add_move(move_list, encode_move(square, to_square, q, 1, 0, 0, 0));
                                    add_move(move_list, encode_move(square, to_square, r, 1, 0, 0, 0));
                                    add_move(move_list, encode_move(square, to_square, b, 1, 0, 0, 0));
                                    add_move(move_list, encode_move(square, to_square, n, 1, 0, 0, 0));
                                }
                                
                                else
                                {
                                    // casual capture
                                    if (board[to_square] >= 1 && board[to_square] <= 6)
                                        add_move(move_list, encode_move(square, to_square, 0, 1, 0, 0, 0));
                                    
                                    // enpassant capture
                                    if (to_square == enpassant)
                                        add_move(move_list, encode_move(square, to_square, 0, 1, 0, 1, 0));
                                }
                            }
                        }
                    }
                }
                
                // black king castling
                if (board[square] == k)
                {
                    // if king side castling is available
                    if (castle & kc)
                    {
                        // make sure there are empty squares between king & rook
                        if (!board[f8] && !board[g8])
                        {
                            // make sure king & next square are not under attack
                            if (!is_square_attacked(e8, white) && !is_square_attacked(f8, white))
                                add_move(move_list, encode_move(e8, g8, 0, 0, 0, 0, 1));
                        }
                    }
                    
                    // if queen side castling is available
                    if (castle & qc)
                    {
                        // make sure there are empty squares between king & rook
                        if (!board[d8] && !board[b8] && !board[c8])
                        {
                            // make sure king & next square are not under attack
                            if (!is_square_attacked(e8, white) && !is_square_attacked(d8, white))
                                add_move(move_list, encode_move(e8, c8, 0, 0, 0, 0, 1));
                        }
                    }
                }
            }
            
            // knight moves
            if (!side ? board[square] == N : board[square] == n)
            {
                // loop over knight move offsets
                for (int index = 0; index < 8; index++)
                {
                    // init target square
                    int to_square = square + knight_offsets[index];
                    
                    // init target piece
                    int piece = board[to_square];
                    
                    // make sure target square is onboard
                    if (!(to_square & 0x88))
                    {
                        //
                        if (
                             !side ?
                             (!piece || (piece >= 7 && piece <= 12)) : 
                             (!piece || (piece >= 1 && piece <= 6))
                           )
                        {
                            // on capture
                            if (piece)
                                add_move(move_list, encode_move(square, to_square, 0, 1, 0, 0, 0));
                                
                            // on empty square
                            else
                                add_move(move_list, encode_move(square, to_square, 0, 0, 0, 0, 0));
                        }
                    }
                }
            }
            
            // king moves
            if (!side ? board[square] == K : board[square] == k)
            {
                // loop over king move offsets
                for (int index = 0; index < 8; index++)
                {
                    // init target square
                    int to_square = square + king_offsets[index];
                    
                    // init target piece
                    int piece = board[to_square];
                    
                    // make sure target square is onboard
                    if (!(to_square & 0x88))
                    {
                        //
                        if (
                             !side ?
                             (!piece || (piece >= 7 && piece <= 12)) : 
                             (!piece || (piece >= 1 && piece <= 6))
                           )
                        {
                            // on capture
                            if (piece)
                                add_move(move_list, encode_move(square, to_square, 0, 1, 0, 0, 0));
                                
                            // on empty square
                            else
                                add_move(move_list, encode_move(square, to_square, 0, 0, 0, 0, 0));
                        }
                    }
                }
            }
            
            // bishop & queen moves
            if (
                 !side ?
                 (board[square] == B) || (board[square] == Q) :
                 (board[square] == b) || (board[square] == q)
               )
            {
                // loop over bishop & queen offsets
                for (int index = 0; index < 4; index++)
                {
                    // init target square
                    int to_square = square + bishop_offsets[index];
                    
                    // loop over attack ray
                    while (!(to_square & 0x88))
                    {
                        // init target piece
                        int piece = board[to_square];
                        
                        // if hits own piece
                        if (!side ? (piece >= 1 && piece <= 6) : ((piece >= 7 && piece <= 12)))
                            break;
                        
                        // if hits opponent's piece
                        if (!side ? (piece >= 7 && piece <= 12) : ((piece >= 1 && piece <= 6)))
                        {
                            add_move(move_list, encode_move(square, to_square, 0, 1, 0, 0, 0));
                            break;
                        }
                        
                        // if steps into an empty squre
                        if (!piece)
                            add_move(move_list, encode_move(square, to_square, 0, 0, 0, 0, 0));
                        
                        // increment target square
                        to_square += bishop_offsets[index];
                    }
                }
            }
            
            // rook & queen moves
            if (
                 !side ?
                 (board[square] == R) || (board[square] == Q) :
                 (board[square] == r) || (board[square] == q)
               )
            {
                // loop over bishop & queen offsets
                for (int index = 0; index < 4; index++)
                {
                    // init target square
                    int to_square = square + rook_offsets[index];
                    
                    // loop over attack ray
                    while (!(to_square & 0x88))
                    {
                        // init target piece
                        int piece = board[to_square];
                        
                        // if hits own piece
                        if (!side ? (piece >= 1 && piece <= 6) : ((piece >= 7 && piece <= 12)))
                            break;
                        
                        // if hits opponent's piece
                        if (!side ? (piece >= 7 && piece <= 12) : ((piece >= 1 && piece <= 6)))
                        {
                            add_move(move_list, encode_move(square, to_square, 0, 1, 0, 0, 0));
                            break;
                        }
                        
                        // if steps into an empty squre
                        if (!piece)
                            add_move(move_list, encode_move(square, to_square, 0, 0, 0, 0, 0));
                        
                        // increment target square
                        to_square += rook_offsets[index];
                    }
                }
            }
        }
    }
}
// COPY/RESTORE BOARD POSITION MACROS
#define copy_board()                                \
    int board_copy[128], king_square_copy[2];       \
    int side_copy, enpassant_copy, castle_copy;     \
    memcpy(board_copy, board, 512);                 \
    side_copy = side;                               \
    enpassant_copy = enpassant;                     \
    castle_copy = castle;                           \
    memcpy(king_square_copy, king_square,8);        \

#define take_back()                                 \
    memcpy(board, board_copy, 512);                 \
    side = side_copy;                               \
    enpassant = enpassant_copy;                     \
    castle = castle_copy;                           \
    memcpy(king_square, king_square_copy,8);        \

// MAKE MOVE
static inline int make_move(int move, int capture_flag)
{
    // QUIET MOVE
    if (capture_flag == all_moves)
    {
        // COPY BOARD STATE
        copy_board();
        
        // PARSE MOVE
        int from_square = get_move_source(move);
        int to_square = get_move_target(move);
        int promoted_piece = get_move_piece(move);
        int enpass = get_move_enpassant(move);
        int double_push = get_move_pawn(move);
        int castling = get_move_castling(move);
	    
        
        // MOVE PIECE
        board[to_square] = board[from_square];
        board[from_square] = e;
        
        // PAWN PROMOTION
        if (promoted_piece)
            board[to_square] = promoted_piece;
        
        // ENPASSANT CAPTURE
        if (enpass)
            !side ? (board[to_square + 16] = e) : (board[to_square - 16] = e);
        
        // RESET ENPASSANT SQUARE
        enpassant = no_sq;
        
        // DOUBLE PAWN PUSH
        if (double_push)
            !side ? (enpassant = to_square + 16) : (enpassant = to_square - 16);
        
        // CASTLING
        if (castling)
        {
            // switch target square
            switch(to_square) {
                // white castles king side
                case g1:
                    board[f1] = board[h1];
                    board[h1] = e;
                    break;
                
                // white castles queen side
                case c1:
                    board[d1] = board[a1];
                    board[a1] = e;
                    break;
               
               // black castles king side
                case g8:
                    board[f8] = board[h8];
                    board[h8] = e;
                    break;
               
               // black castles queen side
                case c8:
                    board[d8] = board[a8];
                    board[a8] = e;
                    break;
            }
        }
        
        // UPDATE KING SQUARE
        if (board[to_square] == K || board[to_square] == k)
            king_square[side] = to_square;
        
        // UPDATE CASTLING RIGHTS
        castle &= castling_rights[from_square];
        castle &= castling_rights[to_square];
        
        // CHANGE SIDE
        side ^= 1;
        
        // TAKE MOVE BACK IF KING IS UNDER THE CHECK
        if (is_square_attacked(!side ? king_square[side ^ 1] : king_square[side ^ 1], side))
        {
            // RESTORE BOARD STATE
            take_back();
            
            // illegal move
            return 0;
        }
        
        else
            // legal move
            return 1;
    }
    
    // capture move
    else
    {
        // if move is a capture
        if (get_move_capture(move))
            // make capture move
            make_move(move, all_moves);
        
        else
            // move is not a capture
            return 0;
    }
}


/***********************************************\

                  PERFT FUNCTIONS (Mostly Copied from Wukong.c by Maksim Korzh)

\***********************************************/

// count nodes
long nodes = 0;

int get_time_ms() {
	struct timeval t;
	gettimeofday (&t, NULL);
	return t.tv_sec*1000 + t.tv_usec/1000;
}

// perft driver
static inline void perft_driver(int depth)
{
    // escape condition
    if  (!depth)
    {
        // count current position
        nodes++;
        return;
    }

    // create move list variable
    moves move_list[1];
    
    // generate moves
    generate_moves(move_list);
    
    // loop over the generated moves
    for (int move_count = 0; move_count < move_list->count; move_count++)
    {
        // copy board state
        copy_board();
        
        // make only legal moves
        if (!make_move(move_list->moves[move_count], all_moves))
            // skip illegal move
            continue;
        
        // recursive call
        perft_driver(depth - 1);
        
        // restore board state
        take_back();
    }
}

// perft test
static inline void perft_test(int depth)
{
    printf("\n    Performance test:\n\n");
    
    // init start time
    int start_time = get_time_ms();

    // create move list variable
    moves move_list[1];
    
    // generate moves
    generate_moves(move_list);
    
    // loop over the generated moves
    for (int move_count = 0; move_count < move_list->count; move_count++)
    {
        // copy board state
        copy_board();
        
        // make only legal moves
        if (!make_move(move_list->moves[move_count], all_moves))
            // skip illegal move
            continue;
        
        // cummulative nodes
        long cum_nodes = nodes;
        
        // recursive call
        perft_driver(depth - 1);
        
        // old nodes
        long old_nodes = nodes - cum_nodes;
        
        // restore board state
        take_back();
        
        // print current move
        printf("    move %d: %s%s%c    %ld\n",
            move_count + 1,
            square_to_coords[get_move_source(move_list->moves[move_count])],
            square_to_coords[get_move_target(move_list->moves[move_count])],
            promoted_pieces[get_move_piece(move_list->moves[move_count])],
            old_nodes
        );
    }
    
    // print results
    printf("\n    Depth: %d", depth);
    printf("\n    Nodes: %ld", nodes);
    printf("\n     Time: %d ms\n\n", get_time_ms() - start_time);
}


/***********************************************\

               EVALUATION FUNCTION

\***********************************************/
// Copied from Wukong.c by Maksim Korzh
// EVALUATION OF THE POSITION
static inline int evaluate_position()
{
    // INIT SCORE
    int score = 0;
    
    // LOOP OVER BOARD SQUARES
    for (int square = 0; square < 128; square++)
    {
        // MAKE SURE SQUARE IS ON BOARD
        if (!(square & 0x88))
        {
            // INIT PIECE
            int piece = board[square];
            
            // MATERIAL SCORE EVALUATION
            score += material_score[piece];
            
            // EVALUATE PIECES
			switch(piece)
			{
				// IF WHITE PAWN
				case P: 
				    // ADD THE POSITIONAL SQUARE
				    score += pawn_score[square];
				    
				    // IF NEXT SQUARE HAS A PAWN (i.e. doubled pawns)
				    if (board[square - 16] == P)
				        score -= 100;
				    
				    break;
				        
                // IF WHITE PIECES
				case N: score += knight_score[square]; break;
				case B: score += bishop_score[square]; break;
				case R: score += rook_score[square]; break;
				case K: score += king_score[square]; break;
				
				// BLACK PAWN
				case p:
				    // ADD POSITIONAL SCORE
				    score -= pawn_score[mirror_score[square]];
				    
				    // DOUBLE PAWNS PENALTY
				    if (board[square + 16] == p)
				        score += 100;
				        
				    break;
                // IF BLACK PIECES
				case n: score -= knight_score[mirror_score[square]]; break;
				case b: score -= bishop_score[mirror_score[square]]; break;
				case r: score -= rook_score[mirror_score[square]]; break;
				case k: score -= king_score[mirror_score[square]]; break;
			}
            
        }
    }

    // RETURN POSITIVE SCORE FOR WHITE & NEGATIVE FOR BLACK
    return !side ? score : -score;
}
//


/***********************************************\

                 SEARCH FUNCTIONS 
)

\***********************************************/

// most valuable victim & less valuable attacker((// Copied from Wukong.c by Maksim Korzh))

/*
                          
    (Victims) Pawn Knight Bishop   Rook  Queen   King
  (Attackers)
        Pawn   105    205    305    405    505    605
      Knight   104    204    304    404    504    604
      Bishop   103    203    303    403    503    603
        Rook   102    202    302    402    502    602
       Queen   101    201    301    401    501    601
        King   100    200    300    400    500    600

*/

static int mvv_lva[13][13] = {
	0,   0,   0,   0,   0,   0,   0,  0,   0,   0,   0,   0,   0,
	0, 105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	0, 104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	0, 103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	0, 102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	0, 101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	0, 100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600,

	0, 105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	0, 104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	0, 103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	0, 102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	0, 101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	0, 100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600
};

// KILLER MOVES [ID][PLY]
int killer_moves[2][64];

// HISTORY MOVES [PIECE][SQUARE]
int history_moves[13][128];

// PV MOVES
int pv_table[64][64];
int pv_length[64];

// HALF MOVE
int ply = 0;

// SCORE MOVE FOR MOVE ORDERING
static inline int score_move(int move)
{
    // PV move
    if (pv_table[0][ply] == move)
        // SCORE 20000 ( SEARCH IT FIRST )
        return 20000;
 
    // INIT CURRENT MOVE SCORE
    int score;
    
    // SCORE MVV LVA (SCORES 0 FOR QUIETE MOVES)
    score = mvv_lva[board[get_move_source(move)]][board[get_move_target(move)]];         

    // ON CAPTURE
    if (get_move_capture(move))
    {
        // add 10000 to current score
        score += 10000;
    }
    
    // on quiete move
    else {
        // on 1st killer move
        if (killer_moves[0][ply] == move)
            // score 9000
            score = 9000;
        
        // on 2nd killer move
        else if (killer_moves[1][ply] == move)
            // score 8000
            score = 8000;
                      
        // on history move (previous alpha's best score)
        else
            // score with history depth
            score = history_moves[board[get_move_source(move)]][get_move_target(move)] + 7000;
    }
    
    // return move score
    return score;
}

static inline void sort_moves(moves *move_list)
{
    // define move scores array
    int move_scores[move_list->count];
    
    // init move scores array
    for (int count = 0; count < move_list->count; count++)
        // score move
        move_scores[count] = score_move(move_list->moves[count]);
    
    // loop over current move score
    for (int current = 0; current < move_list->count; current++)
    {
        // loop over next move score
        for (int next = current + 1; next < move_list->count; next++)
        {
            // order moves descending
            if (move_scores[current] < move_scores[next])
            {
                // swap scores
                int temp_score = move_scores[current];
                move_scores[current] = move_scores[next];
                move_scores[next] = temp_score;
                
                // swap corresponding moves
                int temp_move = move_list->moves[current];
                move_list->moves[current] = move_list->moves[next];
                move_list->moves[next] = temp_move;
            }
        }
    }    
}

// quiescence search (for better understanding, I looked at Chess Programming Wiki article and pseudocode: https://www.chessprogramming.org/Quiescence_Search)
static inline int quiescence_search(int alpha, int beta, int depth)
{
    // update nodes count
    nodes++;
    
    // evaluate position
    int score = evaluate_position();
    
    //  fail hard beta-cutoff
    if (score >= beta)
        return beta;

    // alpha acts like max in MiniMax
    if (score > alpha)
        alpha = eval;

    // create move list variable
    moves move_list[1];
    
    // generate moves
    generate_moves(move_list);
    
    // move ordering
    sort_moves(move_list);
    
    // loop over the generated moves
    for (int count = 0; count < move_list->count; count++)
    {      
        // copy board state
        copy_board();
        
        // increment ply
        ply++;
        
        // ONLY CONSIDER CAPTURES
        if (!make_move(move_list->moves[count], only_captures))
        {
            // decrement ply
            ply--;
            
            // skip illegal move
            continue;
        }
        
        // recursive call (see Chess Programming Wiki article and pseudocode: https://www.chessprogramming.org/Quiescence_Search)
        int score = -quiescence_search(-beta, -alpha, depth); 
        
        // restore board state
        take_back();
        
        // decrement ply
        ply--;
        
        //  fail hard beta-cutoff
        if (score >= beta)
             return beta;
        
        // alpha acts like max in MiniMax
        if (score > alpha)
            alpha = score;
    }
        
    // return alpha score
    return alpha;
	
}

// negamax search (for better understanding, I looked at https://stackoverflow.com/questions/65750233/what-is-the-difference-between-minimax-and-negamax)
static inline int negamax_search(int alpha, int beta, int depth)
{       
    // legal moves
    int legal_moves = 0;
    
    // best move
    int best_so_far = 0;
    
    // old alpha
    int old_alpha = alpha;
    
    // PV length
    pv_length[ply] = ply;

    // escape condition
    if  (depth == 0)
        // search for calm position before evaluation
        return quiescence_search(alpha, beta, depth);

    // update nodes count
    nodes++;
    
    // is king in check?
    int in_check = is_square_attacked(king_square[side], side ^ 1);
    
    // increase depth if king is in check
    if (in_check)
        depth++;
    
    // create move list variable
    moves move_list[1];
    
    // generate moves
    generate_moves(move_list);
    
    // move ordering
    sort_moves(move_list);
    
    // loop over the generated moves
    for (int count = 0; count < move_list->count; count++)
    {
        // copy board state
        copy_board();
        
        // increment ply
        ply++;
        
        // make only legal moves
        if (!make_move(move_list->moves[count], all_moves))
        {
            // decrement ply
            ply--;
            
            // skip illegal move
            continue;
        }
         
        // increment legal moves
        legal_moves++;
        
        // recursive call
        int score = -negamax_search(-beta, -alpha, depth - 1);
        
        // restore board state
        take_back();
        
        // decrement ply
        ply--;

        //  fail hard beta-cutoff
        if (score >= beta)
        {
            // update killer moves
            killer_moves[1][ply] = killer_moves[0][ply];
            killer_moves[0][ply] = move_list->moves[count];
            
            return beta;
        }
        
        // alpha acts like max in MiniMax
        if (score > alpha)
        {
            // update history score
            history_moves[board[get_move_source(move_list->moves[count])]][get_move_target(move_list->moves[count])] += depth;

            // set alpha score
            alpha = score;
            
            // store PV move
			pv_table[ply][ply] = move_list->moves[count];
			
			for (int i = ply + 1; i < pv_length[ply + 1]; i++)
				pv_table[ply][i] = pv_table[ply + 1][i];
	
			pv_length[ply] = pv_length[ply + 1];
            
            // store current best move
            if(!ply)
                best_so_far = move_list->moves[count];
        }      
    }
    
    // if no legal moves
    if (!legal_moves)
    {
        // check mate detection
        if (in_check)
            return -49000 + ply;
        
        // stalemate detection
        else
            return 0;
    }
    
    // return alpha score
    return alpha;
}

// search position
int search_position(int depth)
{
    // init nodes count
    nodes = 0;
    
    // clear PV, killer and history moves
    memset(pv_table, 0, 16384);  // sizeof(pv_table)
    memset(killer_moves, 0, 512);  // sizeof(killer_moves)
    memset(history_moves, 0, 6656);  // sizeof(history_moves)
    
    // best score
    int score;
    
    // iterative deepening
    for (int current_depth = 1; current_depth <= depth; current_depth++)
    {    
        // search position with current depth 
	    score = negamax_search(-50000, 50000, current_depth);
        
        // output best move
        printf("info score cp %d depth %d nodes %ld pv ", score, current_depth, nodes);
        
        // print PV line
        for (int i = 0; i < pv_length[0]; i++)
        {
            printf("%s%s%c ", square_to_coords[get_move_source(pv_table[0][i])],
                              square_to_coords[get_move_target(pv_table[0][i])],
                              promoted_pieces[get_move_piece(pv_table[0][i])]);
        }
        
        printf("\n");
    }
	
	// print best move
    printf("\nbestmove %s%s%c\n", square_to_coords[get_move_source(pv_table[0][0])],
                                  square_to_coords[get_move_target(pv_table[0][0])],
                                  promoted_pieces[get_move_piece(pv_table[0][0])]);
}


/***********************************************\

              UCI PROTOCOL FUNCTIONS (according to UCI Protocol Specification: https://gist.github.com/DOBRO/2592c6dad754ba67e6dcaec8c90165bf )

\***********************************************/

// parse move (from UCI)
int parse_move(char *move_str)
{
    // init move list
	moves move_list[1];
	
	// generate moves
	generate_moves(move_list);

    // parse move string
	int parse_from = (move_str[0] - 'a') + (8 - (move_str[1] - '0')) * 16;
	int parse_to = (move_str[2] - 'a') + (8 - (move_str[3] - '0')) * 16;
	int prom_piece = 0;
    
    // init move to encode
	int move;
	
	// loop over generated moves
	for(int count = 0; count < move_list->count; count++)
	{
	    // pick up move
		move = move_list->moves[count];
        
        // if input move is present in the move list
		if(get_move_source(move) == parse_from && get_move_target(move) == parse_to)
		{
		    // init promoted piece
			prom_piece = get_move_piece(move);

			// if promoted piece is present compare it with promoted piece from user input
			if(prom_piece)
			{
				if((prom_piece == N || prom_piece == n) && move_str[4] == 'n')
					return move;
					
				else if((prom_piece == B || prom_piece == b) && move_str[4] == 'b')
					return move;
					
				else if((prom_piece == R || prom_piece == r) && move_str[4] == 'r')
					return move;
					
				else if((prom_piece == Q || prom_piece == q) && move_str[4] == 'q')
					return move;
					
				continue;
			}
            
            // return move to make on board
			return move;
		}
	}

	// return illegal move
	return 0;
}

// input buffer size
#define inputBuffer (400 * 6)

// UCI driver
void uci()
{
    // init input buffer
	char line[inputBuffer];

//	****BEGIN DERIVED CODE (made changes to UCI driver for XBoard to recognize as DiDaC)****
    // PRINT ENGINE INFO
	printf("id name DiDaC\n");
	printf("id author vds\n");
	printf("uciok\n");
	
	// MAIN LOOP
	while(1)
	{
	    // RESET BUFFER
		memset(&line[0], 0, sizeof(line));
		
		// FLUSH STDOUT
		fflush(stdout);
		
		// SKIP ON NO USER INPUT
		if(!fgets(line, inputBuffer, stdin))
			continue;
	    
	    // SKIP ON EMPTY USER INPUT
		if(line[0] == '\n')
			continue;
	    
	    // PARSE "UCI" COMMAND
		if (!strncmp(line, "uci", 3))
		{
		    // print engine info
			printf("id name DiDaC\n");
			printf("id author vds\n");
			printf("uciok\n");
		}
		
		// *** END DERIVED CODE****
		
		// PARSE "ISREADY" COMMAND
		else if(!strncmp(line, "isready", 7))
		{
		    // return engine is ready
			printf("readyok\n");
			continue;
		}
		
		// PARSE "UCINEWGAME" COMMAND
		else if (!strncmp(line, "ucinewgame", 10))
		{
		    // init board with initial position
			parse_fen(startingFEN);
		}
		
		// PARSE GUI INPUT MOVES AFTER INITIAL POSITION
		else if(!strncmp(line, "position startpos moves", 23))
		{
			// INIT BOARD WITH INITIAL POSITION
			parse_fen(startingFEN);
			print_board();
			
			// INIT MOVE STRING
			char *moves = line;
			
			// INIT OFFSET WHERE THE MOVES START
			moves += 23;
			
			// INIT CHARACTER COUNTER
			int countChar = -1;
			
			// LOOP OVER MOVE STRINGS
			while(*moves)
			{
			    // PICK NEXT MOVE IN THE GUI INPUT
				if(*moves == ' ')
				{
				    // GO TO NEXT MOVE
					*moves++;
					
					// PARSE MOVE AND MAKE IT ON BOARD
					make_move(parse_move(moves), all_moves);
				}
				
				// GO TO NEXT MOVE
				*moves++;
			}
		}
		
		// PARSE INITIAL 
		else if(!strncmp(line, "position startpos", 17))
		{
		    // INIT WITH INITIAL POSITION
			parse_fen(startingFEN);
			print_board();
		}
		
		// INIT BOARD FROM FEN
		else if(!strncmp(line, "position fen", 12))
		{
		    // PARSE FEN INPUT
			char *fen = line;
			fen += 13;
			parse_fen(fen);
			
			// PARSE MOVES AFTER "POSITION FEN" COMMAND
			char *moves = line;
			
			// PARSE "MOVES" COMMAND AFTER "POSITION FEN"
			while(strncmp(moves, "moves", 5))
			{
			    // INCREMENT MOVES' STRING POINTER
				*moves++;
				
				// BREAK WHEN NO MORE MOVES AVAILABLE
				if(*moves == '\0')
					break;
			}
			
			// GO TO MOVES
			moves += 4;
			
			// PARSE MOVES
			if(*moves == 's')
			{
			    // INIT CHARACTER COUNTER
				int countChar = -1;
			    
			    // LOOP OVER INPUT MOVES
				while(*moves)
				{
				    // PICK UP NEXT MOVE
					if(*moves == ' ')
					{
					    // GO TO NEXT MOVE
						*moves++;
						
						// PARSE CURRENT MOVE AND MAKE IT ON BOARD
						make_move(parse_move(moves), all_moves);
					}
				    
				    // GO TO NEXT MOVE
					*moves++;
				}
			}
			
			// PRINT POSITION
			print_board();
		}
		
		// parse fixed depth "go" command
		else if (!strncmp(line, "go depth", 8))
		{
		    // parse "go" command
			char *go = line;
			go += 9;
			
			// parse depth
			int depth = *go - '0';
			
			// search fixed depth
			search_position(depth);
		}
		
		// ***BEGIN DERIVED CODE (changed fixed search to 8 ply instead of 6 in the original)***///
		
		// IF depth is not specified, then use fixed 6 plies or 3 moves. 
		else if (!strncmp(line, "go", 2))
			// search 8 ply fixed
			search_position(8);
		
		// parse "quit" command
		else if(!strncmp(line, "quit", 4))
			break;
		
		// ***END DERIVED CODE***///
	
	}
}


/***********************************************\

                   MAIN DRIVER

\***********************************************/

// main driver
int main()
{
    // run engine in UCI mode
    uci();
    
    return 0;
}








