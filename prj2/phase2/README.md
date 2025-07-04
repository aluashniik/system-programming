make 를 실행하면 ./myshell 이 생성되고, 프로그램을 실행할 수 있다.

이 단계에서는 파이프를 포함한 명령어 실행이 가능해졌다.

다음과 같은 파이프 명령어를 실행할 수 있다:

ls | grep {패턴}: ls의 결과 중에서 특정 패턴을 포함한 줄만 필터링한다.
예: ls -al | grep txt

cat {파일} | sort: 파일 내용을 정렬해서 출력한다.
예: cat test.txt | sort

cat {파일} | grep {패턴} | wc -l: 파이프를 여러 번 연결할 수 있다.
예: cat notes.txt | grep -i hello | wc -l

추가된 주요 기능:
- 파이프 기호(|)가 명령어 안에 포함된 경우, 입력을 나누어 각 명령을 순차적으로 연결하여 처리한다.
- 각 명령마다 자식 프로세스를 fork하여 실행하며, stdout과 stdin을 dup2를 사용해 연결한다.
- 마지막 명령이 종료될 때까지 부모 프로세스는 대기한다.

제한 사항:
- 파일 리다이렉션은 아직 구현하지 않았으며, 파이프만 지원된다.
