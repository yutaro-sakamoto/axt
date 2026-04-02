Run `git diff --cached` and `git status` to understand the staged changes.

Based on the changes, perform the following:

1. If there are unstaged or untracked files that should be included, run `git add` to stage them.
2. Write a commit message following the Conventional Commits specification (https://www.conventionalcommits.org/).
   - Format: `<type>(<scope>): <description>` with an optional body and footer.
   - Types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`, `ci`, `build`, `perf`, etc.
   - The description should be concise, lowercase, and imperative mood.
   - Add a body only when the description alone is insufficient to explain the change.
   - Write entirely in English.
   - Do NOT include any Co-Authored-By trailer or any other indication that this commit was written by an AI.
3. Run `git commit` with the message.
