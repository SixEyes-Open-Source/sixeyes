# Contributing to SixEyes

Thank you for your interest in contributing to SixEyes. This project is safety-critical hardware and software; please follow the steps below.

Before contributing
- Read the authoritative system spec in `SixEyes Technical Reference.txt` and ensure your change does not change architectural decisions.
- Ensure you have signed any required contributor license agreements for your organization (if applicable).

Code guidelines
- Keep changes small and focused; open one PR per feature/bug.
- Follow existing code style and keep firmware changes deterministic.
- Add unit tests or simulation validations where possible.

Safety checklist (required for PRs touching firmware/hardware):
- [ ] Run full simulation and document test setup in the PR.
- [ ] Confirm motors are disabled by default and EN pins remain controlled by the safety task.
- [ ] Verify heartbeat behavior (timeout ~500 ms) and include test logs.
- [ ] Provide wiring/board changes in `electronics/` and update `docs/wiring.md`.

Submitting a PR
- Fork the repo and create a feature branch.
- Create a clear commit history with focused commit messages.
- Reference any issues and include testing steps in the PR description.

Contact
- For safety questions, open an issue with the `safety` label and ping maintainers.
