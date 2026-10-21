---
name: github
description: Issue/PR flow of this repo (the repo is on GitHub, NOT GitLab — glab does not work; gh is not installed on the dev machine, the working path is the REST API). Triggers — "push it", "create a PR", "open a pull request", "create a branch for the task", "write the commit", "merge the PR", "remove Draft", "update the branch to master", "reply to the review".
---

# GitHub issue/PR flow (via the REST API, not `glab`)

Repo: `github.com/ppprok/dual`, default branch **`master`**. The flow is the same as in the team's GitLab projects
(`mlops`, `agents-anatomy`, `oso-ai-chat-v2`), adjusted for GitHub.

`gh` is not installed on the dev machine — we use the REST API. The token (a fine-grained PAT for this repo:
Issues, Pull requests, Contents — read/write) is in `.github.token` at the repo root
(in `.gitignore`, never committed). If the file is missing, ask the user to put it there; do not look for
tokens elsewhere.

```bash
TOKEN=$(tr -d '[:space:]' < .github.token)
API=https://api.github.com/repos/ppprok/dual
H=(-H "Authorization: Bearer $TOKEN" -H "Accept: application/vnd.github+json")
curl -s "${H[@]}" "$API/issues?state=open"
```

**From a git worktree** (`.claude/worktrees/…`, subagents) the token file is not in the working copy — it is in the
main checkout: `TOKEN=$(tr -d '[:space:]' < "$(git rev-parse --path-format=absolute --git-common-dir)/../.github.token")`.

If `gh` appears, it can be used (`GH_TOKEN=$(cat .github.token) gh …`), with the same rules.
Push over SSH (`git@github.com:ppprok/dual.git`) — no token needed.

## Commit identity
Set locally in the repo: `Petr Prokofyev <pprokofyev@onboardsolutions.kz>` — do not change it to another email.

## Language
Issues, PRs, commits, review comments and docs are written in **English**.

## Workflow
1. **Issue before the branch.** `taskID` = the GitHub issue number taken **from the API response** (`.number`),
   not "the next one in order": on GitHub issues and PRs share one numbering. If there is no task, create it first
   with the `issue-template` skill. 1 PR = 1 logical unit.
2. **Branch:** `type/taskID_description` from a fresh `master`, `type` ∈ `feature|bug|task|refactor|docs`,
   description — 2-3 English words. Example: `bug/7_csv-loader-link`. Parallel PRs go in separate git worktrees.
3. **Commit:** `[#taskID] type: Description` — conventional type (`feat|fix|docs|refactor|test|chore|…`),
   description in English, imperative, 1 sentence. **Body — at most 3 sentences.**
   **Do NOT write trailers** (`Co-Authored-By: …`, `Claude-Session: …` — owner's decision, 2026-10-20).
4. **Before the PR:** the checklist from the `build-and-test` skill (build, ctest, `asan` when algorithms change,
   `clang-format`). Whatever you could not verify, say so in the PR and leave that checklist item unchecked.
5. **PR from the template** `.github/pull_request_template.md` (the API does not fill in the template — build the
   body from it):
   - title `[#taskID] - type - Title`; while in preparation — a **draft PR** (`"draft": true`), no `Draft:` prefix needed;
   - `Closes #taskID` in the body (links the issue and auto-closes it on merge into `master`);
   - the checklist as a multi-line task list (`- [ ] item`).

Write the PR/issue body to a file and pass it as JSON from Python — the text contains newlines, quotes and
backticks, and building JSON by hand breaks them:

```bash
git push -u origin bug/7_csv-loader-link
python3 -c 'import json;print(json.dumps({"title":"[#7] - bug - Fix CSVLoader link","head":"bug/7_csv-loader-link",
  "base":"master","draft":True,"body":open("body.md").read()}))' > pr.json
curl -s -X POST "${H[@]}" "$API/pulls" -d @pr.json | python3 -c 'import json,sys;d=json.load(sys.stdin);print(d["number"],d["html_url"])'
```

## State labels
Exactly one on an open task, set by hand: `In progress` (work started, branch created) →
`In review` (there is an open PR waiting for the owner) → on closing, `Done` (merged) or `Rejected`
(closed without being done). `Needs rework` — came back from review; replaced with `In progress` once work on
the rework starts. The PR↔task link is made by the branch name and `Closes #N`, not by mentions in the text.

```bash
curl -s -X PUT "${H[@]}" "$API/issues/<N>/labels" -d '{"labels":["In review"]}'   # replaces the label set
```

Labels on GitHub are created once (`POST $API/labels` with `{"name":"In review","color":"1d76db"}`);
if the needed one is missing, create it rather than using a similar one.

## Merging is up to the user
By default Claude **creates and updates** PRs but does NOT merge them. The owner reviews and merges.

**When the owner explicitly asks to merge:**
1. Update: `git fetch origin && git merge origin/master` (a merge commit), resolve conflicts, push.
2. Remove draft: only via GraphQL — `POST https://api.github.com/graphql` with
   `{"query":"mutation{markPullRequestReadyForReview(input:{pullRequestId:\"<node_id>\"}){clientMutationId}}"}`
   (`node_id` — from `GET $API/pulls/<N>`).
3. Merge: `PUT $API/pulls/<N>/merge` with `{"merge_method":"merge"}`; then delete the branch
   `DELETE $API/git/refs/heads/<branch>` and change the task label to `Done`.

## Replying to review comments
Reply **inside the thread** of the comment, not with a separate comment:
`GET $API/pulls/<N>/comments` → `POST $API/pulls/<N>/comments/<comment_id>/replies` with `{"body":"…"}`.
General PR comments (not attached to a line) — `GET/POST $API/issues/<N>/comments`.
