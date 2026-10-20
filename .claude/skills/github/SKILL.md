---
name: github
description: Issue/PR-флоу этого репо (репо на GitHub, НЕ GitLab — glab не работает; gh на dev-машине не установлен, рабочий путь — REST API). Триггеры — «запушить», «завести PR», «открыть pull request», «создать ветку под задачу», «оформить коммит», «залить/смержить PR», «снять Draft», «актуализировать ветку на master», «ответить на ревью».
---

# GitHub issue/PR-флоу (через REST API, не `glab`)

Репо: `github.com/ppprok/dual`, ветка по умолчанию **`master`**. Флоу тот же, что в GitLab-проектах
команды (`mlops`, `agents-anatomy`, `oso-ai-chat-v2`), с поправками на GitHub.

`gh` на dev-машине не установлен — работаем REST API. Токен (fine-grained PAT на этот репо:
Issues, Pull requests, Contents — read/write) лежит в `.github.token` в корне репо
(в `.gitignore`, в коммиты не попадает). Нет файла — попросить пользователя положить, не искать
токены в других местах.

```bash
TOKEN=$(tr -d '[:space:]' < .github.token)
API=https://api.github.com/repos/ppprok/dual
H=(-H "Authorization: Bearer $TOKEN" -H "Accept: application/vnd.github+json")
curl -s "${H[@]}" "$API/issues?state=open"
```

**Из git-worktree** (`.claude/worktrees/…`, субагенты) файла токена в рабочей копии нет — он лежит в основном
чекауте: `TOKEN=$(tr -d '[:space:]' < "$(git rev-parse --path-format=absolute --git-common-dir)/../.github.token")`.

Если `gh` появится — можно им (`GH_TOKEN=$(cat .github.token) gh …`), правила те же.
Push по SSH (`git@github.com:ppprok/dual.git`) — токен не нужен.

## Идентичность коммитов
В репо задано локально: `Petr Prokofyev <pprokofyev@onboardsolutions.kz>` — не менять на другую почту.

## Порядок работы
1. **Issue раньше ветки.** `taskID` = номер GitHub-issue, взятый **из ответа API** (`.number`),
   не «следующий по порядку»: на GitHub issue и PR делят одну нумерацию. Нет задачи — сначала завести
   по скиллу `issue-template`. 1 PR = 1 логическая единица.
2. **Ветка:** `type/taskID_description` от свежего `master`, `type` ∈ `feature|bug|task|refactor|docs`,
   description — 2-3 англ. слова. Пример: `bug/7_csv-loader-link`. Параллельные PR — в отдельных git-worktree.
3. **Коммит:** `[#taskID] type: Description` — type conventional (`feat|fix|docs|refactor|test|chore|…`),
   description англ., императив, 1 предложение. **Тело — не более 3 предложений.**
   **Трейлеры НЕ писать** (`Co-Authored-By: …`, `Claude-Session: …` — решение владельца, 2026-10-20).
4. **Перед PR:** чек-лист из скилла `build-and-test` (сборка, ctest, при правке алгоритмов — `asan`,
   `clang-format`). Что не удалось проверить — так и написать в PR, не отмечать пункт чек-листа.
5. **PR по шаблону** `.github/pull_request_template.md` (через API шаблон не подставляется — тело
   собирать из него):
   - имя `[#taskID] - type - Title`; пока готовится — **draft PR** (`"draft": true`), префикс `Draft:` не нужен;
   - в теле `Closes #taskID` (линковка + автозакрытие issue при мерже в `master`);
   - чек-лист — многострочным task-list (`- [ ] пункт`).

Тело PR/issue писать в файл и передавать JSON-ом из Python — в тексте переводы строк, кавычки и
обратные апострофы, ручная сборка JSON их портит:

```bash
git push -u origin bug/7_csv-loader-link
python3 -c 'import json;print(json.dumps({"title":"[#7] - bug - Fix CSVLoader link","head":"bug/7_csv-loader-link",
  "base":"master","draft":True,"body":open("body.md").read()}))' > pr.json
curl -s -X POST "${H[@]}" "$API/pulls" -d @pr.json | python3 -c 'import json,sys;d=json.load(sys.stdin);print(d["number"],d["html_url"])'
```

## Метки состояния
Ровно одна на открытой задаче, ставятся вручную: `In progress` (работа начата, ветка заведена) →
`In review` (есть открытый PR, ждёт владельца) → при закрытии `Done` (влито) или `Rejected`
(закрыта без выполнения). `Needs rework` — вернулась с ревью; снимается на `In progress`, когда за
доработку сели. Связь PR↔задача — по имени ветки и `Closes #N`, а не по упоминаниям в тексте.

```bash
curl -s -X PUT "${H[@]}" "$API/issues/<N>/labels" -d '{"labels":["In review"]}'   # заменяет набор меток
```

Метки на GitHub заводятся один раз (`POST $API/labels` с `{"name":"In review","color":"1d76db"}`);
нет нужной — завести, а не ставить похожую.

## Мерж — за пользователем
По умолчанию Claude **заводит и обновляет** PR, но НЕ мержит. Владелец ревьюит и заливает сам.

**Когда владелец явно просит залить:**
1. Актуализация: `git fetch origin && git merge origin/master` (merge-коммит), конфликты зарезолвить, запушить.
2. Снять draft: только GraphQL — `POST https://api.github.com/graphql` с
   `{"query":"mutation{markPullRequestReadyForReview(input:{pullRequestId:\"<node_id>\"}){clientMutationId}}"}`
   (`node_id` — из `GET $API/pulls/<N>`).
3. Мерж: `PUT $API/pulls/<N>/merge` с `{"merge_method":"merge"}`; затем удалить ветку
   `DELETE $API/git/refs/heads/<branch>` и сменить метку задачи на `Done`.

## Ответы на замечания ревью
Отвечать **внутри треда** замечания, а не отдельным комментарием:
`GET $API/pulls/<N>/comments` → `POST $API/pulls/<N>/comments/<comment_id>/replies` с `{"body":"…"}`.
Общие комментарии к PR (не к строке) — `GET/POST $API/issues/<N>/comments`.
