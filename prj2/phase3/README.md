make 를 실행하면 ./myshell 이 생성되고, 프로그램을 실행할 수 있다.

이 단계에서는 백그라운드 실행 및 job control 기능이 추가되었다.

다음과 같은 백그라운드 명령어를 실행할 수 있다:

sleep 10 &: 명령어 끝에 &를 붙이면 백그라운드로 실행된다.
예: sleep 100&, sleep 3 &

jobs: 현재 백그라운드에서 실행 중이거나 중지된 작업 목록을 출력한다.

fg %n: 백그라운드 작업을 포그라운드로 전환한다.
예: fg %1

bg %n: 중지된 작업을 백그라운드에서 다시 실행한다.
예: bg %2

kill %n: 특정 job을 종료한다.
예: kill %3

추가된 주요 기능:
- 명령어 끝에 &가 있는 경우, 백그라운드 job으로 등록되고 즉시 prompt로 복귀한다.
- 각 job은 job 리스트에 관리되며, job id(JID), process id(PID), 상태(Running, Stopped)를 가진다.
- 시그널 핸들러를 통해 Ctrl+C(SIGINT), Ctrl+Z(SIGTSTP) 입력에 대해 포그라운드 job에 시그널 전달이 가능하다.
- SIGCHLD 핸들러를 통해 자식 종료 상태를 비동기적으로 처리하고 job 목록을 갱신한다.

지원되는 job control 기능:
- stop (Ctrl+Z) → ST 상태로 전환
- continue (bg/fg) → SIGCONT 전송
- terminate (Ctrl+C, kill) → SIGINT or SIGKILL 전송
- 다중 파이프 명령어 + 백그라운드 실행도 지원됨

예:
ls -al | grep txt &     → 파이프 + 백그라운드 실행
