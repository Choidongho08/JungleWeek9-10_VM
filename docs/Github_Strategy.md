# 🌳 ThreeJ 팀 GitHub 전략

> 팀원 모두가 이 문서 하나만 보면 됩니다.
> 처음 보는 단어가 나오면 맨 아래 **[📖 용어 사전](#-용어-사전)** 을 먼저 보세요.

**목차**
[1. 한눈에 보기](#1-한눈에-보기) ·
[2. 브랜치 종류와 이름 규칙](#2-브랜치-종류와-이름-규칙) ·
[3. 전체 흐름 그림](#3-전체-흐름-그림) ·
[4. 단계별 작업 방법](#4-단계별-작업-방법) ·
[5. 충돌이 났을 때 (fixed 브랜치)](#5-충돌이-났을-때-fixed-브랜치) ·
[6. 승인 규칙](#6-승인-규칙) ·
[7. 브랜치를 지워도 기록이 남나요?](#7-브랜치를-지워도-기록이-남나요) ·
[8. 하지 말 것](#8-하지-말-것-) ·
[9. 처음 시작하는 팀원](#9-처음-시작하는-팀원) ·
[10. TIL · 회의록 제출](#10-til--회의록-제출) ·
[11. 구축 방법 (관리자)](#11-구축-방법-관리자) ·
[용어 사전](#-용어-사전)

---

## 1. 한눈에 보기

```mermaid
flowchart LR
    K["feature-priority-kim<br/>(김 개인 작업)"] -->|"PR"| F["feature-priority<br/>(기능 하나를 모으는 곳)"]
    L["feature-priority-lee<br/>(이 개인 작업)"] -->|"PR"| F
    X["fixed-priority-lee<br/>(충돌 해결용)"] -.->|"해결 후 넣기"| L
    F -->|"PR + 1명 승인"| D["dev<br/>(팀 코드를 모아 시험)"]
    D -->|"PR + 2명 승인"| M["main<br/>(최종 완성본)"]
```

1. **기능 브랜치** `feature-기능명` 을 하나 만든다
2. 각자 거기서 **개인 브랜치** `feature-기능-이름` 을 따서 작업한다
3. 함수가 완성되면 **로컬에서 테스트** → 기능 브랜치로 **PR**
4. 충돌이 나면 PR 은 열어둔 채 **`fixed-` 브랜치**에서 해결 → 개인 브랜치에 넣기 → PR 머지
5. 기능이 다 모이면 **dev** 로 PR (1명 승인) → dev 에서 테스트
6. 최종으로 **main** 에 PR (2명 승인)

- 원본 Pintos 코드는 `v0-skeleton` **태그**로 영구 보관되어 있습니다.

---

## 2. 브랜치 종류와 이름 규칙

| 종류 | 이름 형식 | 예시 | 누가 | 보호 규칙 | 머지 후 |
|---|---|---|---|---|---|
| 최종본 | `main` | — | 팀 전체 | PR + **2명 승인**, 직접 push 금지 | 영구 보존 |
| 통합 | `dev` | — | 팀 전체 | PR + **1명 승인**, 직접 push 금지 | 영구 보존 |
| 기능 | `feature-<기능>` | `feature-priority` | 팀 전체 공유 | 없음 | dev 에 합쳐져도 **유지** (삭제 X) |
| 개인 작업 | `feature-<기능>-<이름>` | `feature-priority-kim` | 본인 | 없음 | 머지 후 PR 화면 **Delete branch** 로 직접 삭제 |
| 충돌 해결 | `fixed-<기능>-<이름>` | `fixed-priority-lee` | 본인 | 없음 | 해결 후 직접 삭제 |
| TIL 제출 | `til-<MMDD>-<GitHub ID>` | `til-1009-picky232` | 본인 | 없음 | 머지 후 PR 화면 **Delete branch** 로 직접 삭제 |

- 이름은 **영어 소문자 + 하이픈(`-`)** 만 사용 (한글 X)
- 개인 브랜치의 **마지막 단어 = 내 이름**. 기능 이름은 기능 브랜치와 똑같이.
- 레포의 **머지 후 자동 삭제는 꺼져 있습니다.** 기능 브랜치는 남겨 두고, 개인 · TIL 브랜치만 직접 지웁니다.

---

## 3. 전체 흐름 그림

```mermaid
gitGraph
    commit id: "원본" tag: "v0-skeleton"
    branch dev
    checkout dev
    branch feature-priority
    checkout feature-priority
    branch feature-priority-kim
    commit id: "kim: 함수 A 구현"
    checkout feature-priority
    branch feature-priority-lee
    commit id: "lee: 함수 B 구현"
    checkout feature-priority
    merge feature-priority-kim id: "PR #1 머지"
    checkout feature-priority-lee
    branch fixed-priority-lee
    merge feature-priority id: "충돌 해결"
    checkout feature-priority-lee
    merge fixed-priority-lee
    checkout feature-priority
    merge feature-priority-lee id: "PR #2 머지"
    checkout dev
    merge feature-priority id: "PR #3 (1명 승인)"
    checkout main
    merge dev id: "PR #4 (2명 승인)" tag: "priority 완성"
```

---

## 4. 단계별 작업 방법

### ① 기능 시작 — 기능 브랜치 만들기 (기능당 1번, 한 명만)

```bash
git fetch origin && git push origin origin/dev:refs/heads/feature-priority
```

### ② 내 개인 브랜치 만들기

```bash
git fetch origin && git switch -c feature-priority-kim origin/feature-priority
```

### ③ 작업하고 올리기 (매일 퇴근 전 push = 백업 + 진행 공유)

```bash
git add -p && git commit -m "feat: ready list 를 우선순위 순으로 정렬"
```
```bash
git push -u origin HEAD
```

### ④ 함수 완성 → 로컬 테스트 → 기능 브랜치로 PR

```bash
cd pintos/threads && make && cd build && make check
```
```bash
gh pr create --base feature-priority --fill
```
(GitHub 웹에서 **Compare & pull request** → base 를 `feature-priority` 로 골라도 됨)

> ⚠️ 기본 브랜치가 `main` 이라서 웹에서 PR 을 만들면 **base 가 `main` 으로 잡혀 있습니다.** 반드시 바꾸세요.

- 승인 없이 **본인이 바로 머지** 가능 → **Create a merge commit** 클릭
- 충돌 표시가 뜨면 → [5. 충돌이 났을 때](#5-충돌이-났을-때-fixed-브랜치)
- 머지되면 PR 화면의 **Delete branch** 버튼으로 내 개인 브랜치 삭제 (자동 삭제 안 됨). 다음 함수는 ② 부터 다시 (최신 기능 브랜치에서 새로 따기)

### ⑤ 기능 완성 → dev 로 PR (1명 승인)

먼저 **이 기능 브랜치로 열린 개인 PR 이 하나도 없는지** 확인:
```bash
gh pr list --base feature-priority
```
비어 있으면 PR 생성:
```bash
gh pr create --base dev --head feature-priority --fill
```
- 팀원 1명이 승인 → 머지 → `feature-priority` 는 **지우지 않고 유지**
- 머지 후 dev 를 받아서 테스트:
  ```bash
  git fetch origin && git switch dev && git pull && cd pintos/threads && make && cd build && make check
  ```

### ⑥ 최종 → main 으로 PR (2명 승인)

```bash
gh pr create --base main --head dev --fill
```
- **나를 뺀 팀원 2명 모두** 승인 → 머지

---

## 5. 충돌이 났을 때 (fixed 브랜치)

**충돌(conflict)** = 내가 고친 줄을 다른 팀원도 고쳐서 먼저 머지한 상황.
PR 화면에 *"This branch has conflicts that must be resolved"* 가 뜹니다.

```mermaid
sequenceDiagram
    actor 나
    participant pr as 열려 있는 PR
    participant mine as feature-priority-lee
    participant fix as fixed-priority-lee
    participant feat as feature-priority
    Note over pr: ❌ 충돌 — PR 은 닫지 말고 그대로 둠
    나->>fix: ① 내 개인 브랜치에서 fixed 브랜치 만들기
    feat->>fix: ② 최신 기능 브랜치를 fixed 로 합치기 (충돌 발생)
    나->>fix: ③ 충돌 정리 + 로컬 테스트
    fix->>mine: ④ fixed 를 내 개인 브랜치에 합치고 push
    mine->>pr: ⑤ PR 이 자동으로 갱신됨 → ✅ 충돌 없음
    pr->>feat: ⑥ 머지
```

**① fixed 브랜치 만들기** (내 개인 브랜치에서)
```bash
git switch feature-priority-lee && git switch -c fixed-priority-lee
```

**② 최신 기능 브랜치 합치기** → 충돌 발생
```bash
git fetch origin && git merge origin/feature-priority
```

**③ 충돌 정리**
충돌난 파일을 열어 `<<<<<<<` ~ `=======` ~ `>>>>>>>` 사이를 올바른 코드로 고치고 표시 줄은 지웁니다.
VSCode 에서는 *Accept Current / Incoming / Both* 버튼으로 골라도 됩니다. 그다음 저장 →
```bash
git add . && git commit
```
로컬 테스트:
```bash
cd pintos/threads && make && cd build && make check
```

**④ 내 개인 브랜치에 넣고 push**
```bash
git switch feature-priority-lee && git merge fixed-priority-lee && git push
```

**⑤ PR 화면 새로고침** → 충돌 표시가 사라지고 머지 버튼 활성화 → 머지

**⑥ fixed 브랜치 정리**
```bash
git branch -d fixed-priority-lee
```

> 💡 **왜 바로 개인 브랜치에서 안 고치고 fixed 를 거치나요?**
> 충돌 정리를 잘못해도 내 개인 브랜치는 멀쩡하게 남아 있어서, fixed 만 지우고 다시 시도하면 됩니다.

---

## 6. 승인 규칙

| 합치는 방향 | 필요한 승인 | 누가 머지 |
|---|---|---|
| 개인 → 기능 (`feature-기능-이름` → `feature-기능`) | 없음 | 본인 |
| fixed → 개인 | 없음 (로컬에서 합침) | 본인 |
| 기능 → dev | **1명** | 승인 받은 뒤 작성자 |
| dev → main | **나를 뺀 2명** (= 나머지 팀원 전원) | 승인 받은 뒤 작성자 |
| TIL → dev (`til-MMDD-ID` → `dev`) | **1명** | 승인 받은 뒤 작성자 |

- GitHub 는 **PR 작성자 본인의 승인은 세지 않습니다.**
- 승인 뒤에 코드를 또 push 하면 **승인이 취소**되고 다시 받아야 합니다 (dev, main).
- 레포 관리자(picky232)는 **우회(bypass) 권한**으로 승인 없이 머지할 수 있습니다. 단, **main · dev 삭제는 관리자도 불가**합니다.
- 리뷰 부탁: PR 오른쪽 **Reviewers** 에서 지정 + 팀 채팅에 PR 링크 공유.
- **main 은 dev 에서만** PR 합니다. (시스템이 막지는 않으니 팀 약속으로 지키기)

---

## 7. 브랜치를 지워도 기록이 남나요?

**네. 머지된 브랜치는 지워도 커밋 기록이 그대로 남습니다.**

- 브랜치는 커밋을 가리키는 **이름표**일 뿐입니다. 머지하면 커밋들이 기능 브랜치 → dev → main 기록 안으로 들어가므로, 이름표를 떼도 커밋은 남습니다.
- merge commit 메시지에 **브랜치 이름이 영구히** 남습니다.
  예: `Merge pull request #12 from picky232/feature-priority-kim`
- **PR 페이지도 영구 보존**됩니다 (커밋 목록, 코멘트, 변경 내용). 필요하면 PR 화면의 **Restore branch** 로 브랜치를 되살릴 수 있습니다.
- 브랜치 삭제는 **머지된 PR 의 브랜치만** 하세요. PR 화면의 **Delete branch** 버튼은 머지된 뒤에만 나타나므로 안전합니다.
- 단, **머지 안 한 브랜치를 직접 지우면** 그 커밋은 사라질 수 있으니 주의.

누가 무엇을 했는지 보기:
```bash
git log --oneline --graph --first-parent dev
```
```bash
git log --oneline --merges
```

---

## 8. 하지 말 것 🚫

| 하지 말 것 | 이유 |
|---|---|
| `main` / `dev` 에 직접 push | 막혀 있음. 반드시 PR |
| 개인 PR 이 열린 채로 기능 → dev 머지 | 아직 안 합쳐진 개인 작업이 빠진 채로 dev 에 올라감 |
| 기능 브랜치 `feature-<기능>` 삭제 | 팀이 계속 쓰는 브랜치. 머지 후에도 유지 |
| `feature-*` → `main` 바로 PR | dev 에서 시험을 안 거친 코드가 최종본에 들어감 |
| 테스트 안 하고 PR | 자동 테스트가 없어서 내가 확인 안 하면 아무도 모름 |
| `printf` 디버그 출력 남기고 PR | Pintos 테스트는 출력 비교라서 FAIL 됨 |
| 남의 개인 브랜치에 push | 그 사람 작업과 꼬임 |
| 같은 함수를 말없이 동시에 수정 | 충돌 발생. 작업 전에 공유 |
| 충돌 났다고 PR 닫기 | fixed 로 해결하면 같은 PR 그대로 머지 가능 |
| PR 의 base 확인 안 하기 | 기본값이 `main` 이라 실수로 최종본에 올라갈 수 있음 |

---

## 9. 처음 시작하는 팀원

- [ ] 메일 또는 GitHub 알림으로 온 **레포 초대 수락**
- [ ] 레포 받기 — **폴더 이름을 꼭 `pintos_22.04_lab_docker` 로** (개발 컨테이너 설정이 이 이름을 씀)
  ```bash
  git clone https://github.com/picky232/ThreeJ-team-Pintos.git pintos_22.04_lab_docker
  ```
  clone 하면 `main`(최종본)이 열립니다. 작업 브랜치는 항상 `origin/dev` 나 `origin/feature-*` 에서 따세요.
- [ ] VSCode 로 폴더 열기 → 왼쪽 아래 `><` 버튼 → **Reopen in Container**
- [ ] 컨테이너 터미널에서 내 정보 설정
  ```bash
  git config --global user.name "내이름" && git config --global user.email "깃허브이메일"
  ```
- [ ] 팀원과 **누가 어떤 기능/함수를 맡을지** 나누기

---

## 10. TIL · 회의록 제출

매일 AI 와 학습 · 작업한 내용을 md 로 정리해서 팀원과 공유합니다. (논의: #6)

### 폴더 구조

```
docs/
└── 2026-10-09/
    ├── TIL/
    │   ├── picky232.md
    │   ├── Jongeume.md
    │   └── jaeyun-sw-ai.md
    ├── scrum.md        ← 스크럼 회의록
    └── coretime.md     ← 코어타임 회의록
```

- TIL 파일 이름 = **내 GitHub ID** (대소문자 그대로, 예: `Jongeume.md`)
- **1인 1일 1파일.** AI 세션을 여러 개 썼으면 세션 내용을 **하나로 합쳐서** 제출
- 회의록은 날짜 폴더 바로 아래에 회의 1건 = 파일 1개 (기록자가 작성)
- 그날 폴더가 없으면 직접 만들기: `docs/YYYY-MM-DD/TIL/`

### 제출 흐름

```mermaid
flowchart LR
    A["til-1009-picky232"] -->|"PR + 1명 승인<br/>TIL : 2026-10-09 picky232"| D["dev"]
    B["til-1009-jongeume"] -->|"PR + 1명 승인"| D
    C["til-1009-jaeyun-sw-ai"] -->|"PR + 1명 승인"| D
    D -->|"하루 1번 PR + 2명 승인<br/>TIL : 2026-10-09"| M["main"]
```

**① TIL 브랜치 만들기** (항상 `origin/dev` 에서)
```bash
git fetch origin && git switch -c til-1009-picky232 origin/dev
```
- 브랜치 이름: `til-<MMDD>-<GitHub ID>`. 브랜치는 **소문자만** 쓰므로 ID 에 대문자가 있으면 소문자로 (`Jongeume` → `til-1009-jongeume`)

**② 파일 작성** → `docs/2026-10-09/TIL/picky232.md`

**③ 커밋 + push**
```bash
git add docs && git commit -m "docs: TIL 2026-10-09 picky232" && git push -u origin HEAD
```

**④ dev 로 PR** — 제목 형식 `TIL : <날짜> <GitHub ID>`
```bash
gh pr create --base dev --title "TIL : 2026-10-09 picky232" --body "Refs #6"
```
- 팀원 1명이 읽고 승인 → 머지 → PR 화면 **Delete branch**
- 회의록(`scrum.md`, `coretime.md`)은 기록자가 자기 TIL PR 에 같이 넣거나, 같은 방식으로 따로 PR

**⑤ 하루 마무리 — dev → main PR** — 제목 형식 `TIL : <날짜>`
```bash
gh pr create --base main --head dev --title "TIL : 2026-10-09" --body "Refs #6"
```
- 나를 뺀 2명 승인 → 머지
- **그날 TIL PR 이 모두 dev 에 머지된 뒤에** 열기. 열린 뒤 dev 에 새로 머지되면 main 승인이 취소되어 다시 받아야 함
- `dev → main` PR 은 한 번에 하나만 열 수 있음. **전날 PR 이 머지됐는지** 먼저 확인 (`gh pr list --base main`)
- ⚠️ 그날 dev 에 들어간 **코드 변경도 같이 main 으로 갑니다.** dev 가 테스트를 통과한 상태인지 확인하고 올리기

---

## 11. 구축 방법 (관리자)

> 이 레포는 아래 설정이 **이미 모두 적용**되어 있습니다. 다음에 새 레포를 만들 때 참고하세요.
> 필요한 것: 레포 **관리자 권한**, 레포 **공개(Public)** 유지 (개인 무료 계정은 공개 레포에서만 보호 규칙 사용 가능)

| 단계 | 내용 | 상태 |
|---|---|---|
| ① | 원본 태그 + dev 만들기 | ✅ |
| ② | 레포 기본 설정 | ✅ |
| ③ | 보호 규칙 (main 2명 / dev 1명 + main·dev 삭제 금지) | ✅ |
| ④ | 팀원 초대 | ✅ |

### ① 원본 태그 + dev 만들기

```bash
git tag v0-skeleton a345ec1 && git push origin v0-skeleton
```
```bash
git switch -c dev && git push -u origin dev
```

### ② 레포 기본 설정 — Settings → General

| 위치 | 설정 | 이유 |
|---|---|---|
| Default branch | `main` | 레포 첫 화면에 최종본이 보임. 이슈·PR 템플릿도 main 에 있어야 적용됨 |
| Pull Requests | ✅ Allow merge commits | 세부 커밋 기록 보존 |
| Pull Requests | ⬜ Allow squash merging | 커밋이 하나로 뭉쳐져 기록이 사라짐 |
| Pull Requests | ⬜ Allow rebase merging | 기록이 다시 써짐 |
| Pull Requests | ⬜ Automatically delete head branches | 레포 전체에 적용돼서 기능 브랜치까지 지워짐. 개인 · TIL 브랜치는 직접 삭제 |

터미널로 한 번에:
```bash
gh api -X PATCH repos/picky232/ThreeJ-team-Pintos -F delete_branch_on_merge=false -F allow_squash_merge=false -F allow_rebase_merge=false -F allow_merge_commit=true -f default_branch=main
```

> 자동 삭제는 브랜치별로 켜고 끌 수 없어서, `feature-<기능>` 을 남기려고 **레포 전체에서 끕니다.**

### ③ 보호 규칙 — Settings → Rules → Rulesets → New branch ruleset

**세 개**를 만듭니다. 모두 Enforcement **Active**.

| 항목 | `protect-main` | `protect-dev` | `keep-main-dev` |
|---|---|---|---|
| Target → Include by pattern | `main` | `dev` | `main`, `dev` |
| Bypass list | **Repository admin** (Always allow) | **Repository admin** (Always allow) | **비움** ← 중요 |
| Restrict deletions (삭제 금지) | ✅ | ✅ | ✅ |
| Block force pushes (강제 push 금지) | ✅ | ✅ | — |
| Require a pull request before merging | ✅ | ✅ | — |
| └ Required approvals | **2** | **1** | — |
| └ Dismiss stale approvals when new commits are pushed | ✅ | ✅ | — |
| └ Allowed merge methods | Merge 만 | Merge 만 | — |

`feature-*`, `fixed-*` 에는 규칙을 만들지 않습니다 (자유롭게 push, 승인 없이 머지).

> ⚠️ **bypass(우회) 주의 — 왜 `keep-main-dev` 가 따로 있나요?**
> bypass 는 규칙 하나가 아니라 **그 규칙 묶음 전체**를 건너뜁니다.
> `protect-main` / `protect-dev` 에 관리자 우회를 넣으면 승인뿐 아니라 **삭제 금지도 관리자에게는 꺼집니다.**
> 예전에 "머지 후 자동 삭제" 를 켜 뒀을 때, 관리자가 `dev → main` 을 머지하자 **dev 가 자동 삭제**됐습니다 (자동 삭제는 머지한 사람 권한으로 실행됨).
> 지금은 자동 삭제를 껐지만, 관리자의 실수 삭제까지 막으려고 삭제 금지만 담은 **우회 없는 규칙**(`keep-main-dev`)을 따로 둡니다.
> 관리자에게도 반드시 걸려야 하는 규칙은 항상 **우회 없는 별도 규칙**으로 만드세요.

<details>
<summary>터미널로 만들기 (gh)</summary>

`protect-dev` (관리자 우회 포함). `protect-main` 은 `dev` → `main`, `1` → `2` 로 바꿔서 한 번 더 실행.

```bash
gh api repos/picky232/ThreeJ-team-Pintos/rulesets --input - <<'EOF'
{
  "name": "protect-dev",
  "target": "branch",
  "enforcement": "active",
  "bypass_actors": [ { "actor_id": 5, "actor_type": "RepositoryRole", "bypass_mode": "always" } ],
  "conditions": { "ref_name": { "include": ["refs/heads/dev"], "exclude": [] } },
  "rules": [
    { "type": "deletion" },
    { "type": "non_fast_forward" },
    { "type": "pull_request", "parameters": {
        "required_approving_review_count": 1,
        "dismiss_stale_reviews_on_push": true,
        "require_code_owner_review": false,
        "require_last_push_approval": false,
        "required_review_thread_resolution": false,
        "allowed_merge_methods": ["merge"] } }
  ]
}
EOF
```

`keep-main-dev` (우회 없음):
```bash
gh api repos/picky232/ThreeJ-team-Pintos/rulesets --input - <<'EOF'
{
  "name": "keep-main-dev",
  "target": "branch",
  "enforcement": "active",
  "bypass_actors": [],
  "conditions": { "ref_name": { "include": ["refs/heads/main", "refs/heads/dev"], "exclude": [] } },
  "rules": [ { "type": "deletion" } ]
}
EOF
```

현재 규칙 확인 (규칙마다 따로 조회해야 bypass 개수가 정확히 나옴):
```bash
for id in $(gh api repos/picky232/ThreeJ-team-Pintos/rulesets --jq '.[].id'); do gh api repos/picky232/ThreeJ-team-Pintos/rulesets/$id --jq '"\(.name) \(.enforcement) bypass=\(.bypass_actors | length) \(.conditions.ref_name.include)"'; done
```
정상이면:
```
keep-main-dev active bypass=0 ["refs/heads/main","refs/heads/dev"]
protect-dev active bypass=1 ["refs/heads/dev"]
protect-main active bypass=1 ["refs/heads/main"]
```

삭제 금지 확인 — **거부되어야 정상** (`Bypassed` 문구 없이 `! [remote rejected]`):
```bash
git push origin --delete dev
```
</details>

### ④ 팀원 초대 — Settings → Collaborators → Add people

팀원 GitHub ID 입력 → 초대 → 팀원이 **수락해야** 승인 가능.

### 운영 팁

| 상황 | 할 일 |
|---|---|
| 규칙을 잠깐 꺼야 할 때 | Ruleset → Enforcement **Disabled** → 작업 → 다시 **Active** |
| 한 명이 장기 결석이라 main 머지가 막힐 때 | `protect-main` 승인 수를 잠깐 1 로 낮추고 팀에 공유 |
| 팀원이 바뀔 때 | ④ 에서 초대 / 제거 |

---

## 📖 용어 사전

| 용어 | 쉬운 설명 |
|---|---|
| **브랜치 (branch)** | 코드의 "평행 세계". 다른 사람 코드에 영향 없이 따로 작업하는 공간 |
| **커밋 (commit)** | 작업 내용을 저장하는 "세이브 포인트" |
| **push** | 내 컴퓨터의 커밋을 GitHub 에 올리기 |
| **fetch** | GitHub 의 최신 상태를 내 컴퓨터로 가져오기 (내 코드는 안 바뀜) |
| **pull** | fetch + 내 브랜치에 바로 합치기 |
| **PR (Pull Request)** | "내 브랜치를 저기에 합쳐 주세요" 라는 요청서. 리뷰와 승인이 여기서 이뤄짐 |
| **base / head** | PR 에서 base = 합쳐질 곳(받는 쪽), head = 내 브랜치(보내는 쪽) |
| **머지 (merge)** | 두 브랜치의 코드를 하나로 합치기 |
| **merge commit** | 합칠 때 "언제 무엇을 합쳤다" 는 기록을 남기는 방식. 세부 커밋도 모두 보존 |
| **squash / rebase** | 커밋을 뭉치거나 다시 쌓는 합치기 방식. 기록이 바뀌어서 **우리 팀은 안 씀** |
| **충돌 (conflict)** | 두 사람이 같은 줄을 다르게 고쳐서 Git 이 자동으로 못 합치는 상황 |
| **리뷰 / 승인 (approve)** | 다른 팀원이 코드를 보고 "합쳐도 된다" 고 확인해 주는 것 |
| **보호 규칙 (Ruleset)** | 브랜치별 규칙. 직접 push 금지, 승인 필요, 삭제 금지 등 |
| **Enforcement** | 규칙을 켤지(Active) 끌지(Disabled) 정하는 스위치 |
| **Bypass list** | 규칙을 무시해도 되는 사람 목록. 비워두면 예외 없음 |
| **force push (강제 push)** | GitHub 기록을 내 것으로 덮어쓰기. 기록이 사라질 수 있어 main · dev 는 금지 |
| **태그 (tag)** | 특정 커밋에 붙이는 영구 이름표. `v0-skeleton` = 원본 코드 |
| **Default branch** | 레포를 열거나 PR 을 만들 때 기본으로 선택되는 브랜치 |
| **Collaborator** | 레포에 쓰기 권한이 있는 사람 |
| **gh (GitHub CLI)** | GitHub 를 터미널에서 다루는 공식 도구 (`gh auth login` 으로 로그인) |
| **make check** | Pintos 공식 테스트 전체를 실행하는 명령 |
| **PASS / FAIL** | Pintos 테스트 통과 / 실패 |
| **TIL (Today I Learned)** | 오늘 배운 내용을 정리한 기록. 우리 팀은 `docs/날짜/TIL/GitHub ID.md` 로 매일 제출 |
