# DiDac
This project is a semester-long attempt to build a competitive chess engine in C, as well as including/comparing search optimatizations and their effect on the efficiency of the resultant engine. As the purpose of the engine is didactic, i.e. accessible and well-formulated code available to all seekers, its name is simply DiDaC.

## What are the main features of DiDaC?

DiDaC is simple yet contains the following features:

- Alpha beta search/Negamax
- Iterative deepening
- Quiescence search
- MVV/LVA move ordering
- Basic evaluation using PSTs and Positional Matrices
- Basic Killer and History Heuristic
- UCI Protocol (for playing other engines)

## Key Files

There is just one C file titled "DiDaC.c". It entails all search, evaluation and board set up code necessary. I thought having a one-page document with explicit comments for each functionality was ideal for easy accessibility. You can get the engine on a local GUI like XBoard (what I used) or WinBoard by the following process:

- Clone the repository
- Compile "DiDaC.c" to form an executable, i.e. gcc DiDaC.c -o DiDaC
- Download XBoard (https://www.gnu.org/software/xboard/)
- Go to XBoard; Under "Engine" click "Load New First Engine"
- Specify the directory, and the executable under "Engine Command"
- Click the checkbox "UCI Protocol" and "Force current variant under this engine"
- Done! You can now play against it (change modes under "Mode" section) or enroll in arena tournaments. 

## Acknowledgements

Lot of sources have influenced me and the code. These include YouTube creators such as BlueFeverSoftware, especially through his "Vice" Chess Engine series, where he discussed extensively how to implement the optimizations as well as live demos of playing it against other engines through WinBoard (which helped me a lot!). Next comes the "Wukong.JS" series by Maksim Korzh. This pretty much provided the entire skeleton for my project, saving me countless hours of work, although it was in a different Javascript language. I've cited sources from Chess Programming Wiki (e.g. their pseudocode for Quiescence search) as well as StackExchange threads throughout my code (citations usually start as "Adapted/Inspired from..." etc). 

## Citations

1. Korzh, M. (2021). Playlists Chess Programming. YouTube. Retrieved May 1, 2023, from [https://www.youtube.com/playlist?list=PLmN0neTso3JyHz4YvcQrfS7pd9pC_BFBc]
2. Allbert, R. (2013). Playlists [YouTube channel]. YouTube. Retrieved May 1, 2023, from [https://www.youtube.com/playlist?list=PLZ1QII7yudbc-Ky058TEaOstZHVbT-2hg]
3. Korzh, M. (2021). Wukong JS: Didactic javascript chess engine. Ukraine: GitHub; [accessed May 1, 2023]. (https://github.com/maksimKorzh/wukongJS)
4. Korzh, M. (2021). Wukong: Modular & didactic UCI chess engine written by Code Monkey King. Ukraine: GitHub; [accessed May 1, 2023]. [https://github.com/maksimKorzh/wukong]
5. Allbert, R. (2013). Vice: Repo for the Vice chess engine series on YouTube. GitHub; accessed May 1, 2023. [https://github.com/bluefeversoft/vice/tree/main].

