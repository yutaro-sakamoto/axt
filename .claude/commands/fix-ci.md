Fix CI failures on the current branch.

1. Run `gh run list --branch $(git branch --show-current) --limit 5` to find the latest workflow run.
2. If the run is still in progress, poll with `gh run watch <run-id>` until it completes.
3. If the run succeeded, report that and stop.
4. If the run failed, investigate:
   - Run `gh run view <run-id>` to identify which jobs failed.
   - Run `gh run view <run-id> --log-failed` to get the failure logs.
   - Analyze the logs to determine the root cause.
5. Fix the code to resolve the failure.
6. Run the corresponding `make` commands locally to verify the fix.
7. Repeat from step 5 if the local verification fails.
