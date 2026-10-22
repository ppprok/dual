---
name: issue-template
description: Template and rules for creating tasks in this repo's GitHub Issues (Context / DoD / Plan / Constraints and dependencies). Triggers — "create a task", "create an issue", "make this a task", "split into tasks", "need a ticket", "add to the backlog".
---

# How to create tasks

A task is written for whoever picks it up **a month from now and without you**: why it exists, how to tell
it is done, and what they may trip over.

Issues are written in **English** (as are PRs, commits, review comments and docs).

## Template

```markdown
## Context
Why the task exists: 1-3 sentences. Where it came from, what happens if it is not done.

## DoD
What we will see when the task is done — verifiably:
- artifacts (code in a branch, a test, a library/wrapper, an experiment report);
- measurable criteria where applicable (number of covers on a test matrix, enumeration time ≤ X);
- for research: what counts as confirming the hypothesis, and what as refuting it.
Out of scope: …

## Plan
A plan in bullet points (one sentence for small tasks). The forks in the road that will have to be decided along the way.

## Constraints and dependencies
Blocking tasks (`#N`), external dependencies, decisions needed from the owner.
```

## How to fill it in

- **Context** — the consequence of not acting, not the importance.
- **DoD** — the main section. For dualization algorithms the guarantee is usually checked by a test on a matrix
  with a known number of minimal covers (`src/tests/data/*.hg`, name `rows_cols_count`): state the
  guarantee ("RUNCM finds exactly 275 covers on `15_20_275.hg` with batched output"), not the implementation.
  Name separately what is NOT included.
- **Plan** — the value is in the forks: name them, so that the implementer does not decide silently.
- **Constraints** — anything irreversible or external (a publication, a change of the `librunc.so` API that
  the Python code depends on) is a blocker, visible before work starts.

## Command

Body to a file, JSON built with Python (see the `github` skill):

```bash
TOKEN=$(tr -d '[:space:]' < .github.token)
python3 -c 'import json;print(json.dumps({"title":"Short gist of the task","body":open("body.md").read()}))' > issue.json
curl -s -X POST -H "Authorization: Bearer $TOKEN" -H "Accept: application/vnd.github+json" \
  https://api.github.com/repos/ppprok/dual/issues -d @issue.json \
  | python3 -c 'import json,sys;d=json.load(sys.stdin);print(d["number"],d["html_url"])'
```

**Take the task number from the response** — not from memory and not "the next one": issues and PRs share
numbering on GitHub.

## Relation to the rest of the flow

The task is created **before** the branch: branch `type/taskID_description`, commits `[#taskID] type: Description`,
`Closes #taskID` in the PR body (the `github` skill). A task that is never done is fine; a branch without a task is not.
Something big becomes an umbrella task with a list of slices; each slice is useful on its own.
