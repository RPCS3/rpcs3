# Getting Started

Before getting started using the emulator, read the [Quickstart Guide](https://rpcs3.net/quickstart). After reading it, if you need support, check out [How to ask for Support](https://github.com/RPCS3/rpcs3/wiki/How-to-ask-for-Support).

# Issue Reporting

**The GitHub Issue Tracker is not the place to ask for support or to submit [Game Compatibility](https://rpcs3.net/compatibility) reports.** Requests for support or incorrect reports will be closed. If you are not sure whether the issue you want to report is an actual issue that is not yet known, please use the forums to submit such report.

**Before reporting an issue:**
- Check if your system matches all the minimum requirements listed in the [Quickstart Guide](https://rpcs3.net/quickstart);
- Search older issues/forum threads to see if your issue was already submitted;
- Use understandable English. It doesn't need to be perfect, but clear enough to understand your message;
- While reporting issues, please follow the template for the type of issue you've selected (Regression Report, Bug Report or Feature Request), which is prefilled on the issue's textbox.

Submitting your test results for Commercial Games must be done on our forums. Please read the [Game Compatibility](https://github.com/RPCS3/rpcs3/wiki/Game-Compatibility) wiki page before doing so.

# Contributing

Thank you for wanting to help improve RPCS3! Check the [Coding Style Guidelines](https://github.com/RPCS3/rpcs3/wiki/Coding-Style) and [Developer Information](https://github.com/RPCS3/rpcs3/wiki/Developer-Information). If you have any questions, hit us up on our [Discord Server](https://discord.gg/rpcs3) in the **#development** channel.

## Talk to us before you write code

RPCS3 is maintained by a small team of volunteers, and every pull request takes real time to review, test across many games and hardware configurations, and maintain afterwards. A change that looks simple can have side effects that only someone familiar with that part of the emulator will spot.

So if you have a new idea, fix or feature in mind, **please discuss it with us first** instead of opening a pull request straight away:

- Reach out to the developers in the **#development** channel on our [Discord Server](https://discord.gg/rpcs3), or
- [Open an issue](https://github.com/RPCS3/rpcs3/issues/new/choose) (or comment on an existing one) describing what you want to change, why, and how you plan to approach it.

This lets us tell you early whether the change is wanted, whether someone is already working on it, and what pitfalls to look out for. It saves you from spending effort on something that cannot be merged, and it saves us from reviewing it.

## Pull requests from first-time contributors

To keep the review workload manageable, pull requests are checked automatically when they are opened:

- **If you have had a commit merged into RPCS3 before**, or you are a member or collaborator of the project, you can open pull requests as usual.
- **If this is your first contribution**, your pull request must be linked to an issue that a maintainer has **assigned to you**. Pull requests without one are closed automatically.

The process for new contributors is:

1. Discuss your idea on Discord or in a GitHub issue, as described above.
2. Once a maintainer agrees with the approach, they will assign the issue to you. If the discussion happened on Discord, an issue will be created (or you can create one) so it can be assigned.
3. Open your pull request against the `master` branch and include `Fixes #<issue number>` (or `Closes #<issue number>`) in its description.

If your pull request was closed by the automation, you do not need to open a new one. Once the issue is assigned to you, edit the pull request's description to add the issue reference and it will be reopened automatically.

Once your first pull request has been merged, the check no longer applies to you.

## Testing

Please test your changes before submitting them. At minimum, the code must build and the emulator must run the games or features affected by your change. Describe in the pull request what you tested and on which hardware and operating system. If your change affects rendering, CPU emulation or other areas that impact many games, test more than one title.

## AI Use

Use of AI tools for research and reverse engineering purposes is permitted. However, contributors are expected to fully own and understand all code they submit. Any communication with the team — including code, code comments, and GitHub comments — must come from the human contributor, not an AI agent acting autonomously.

We have unfortunately seen a rise in untested and unverified AI-generated slop being submitted to this project. This wastes maintainer time and, in worse cases, such changes get merged and break functionality for all users. Repeated violations will result in a ban from the repository. Please be respectful of everyone's time.

**Pull requests opened by AI agents or automated tools must include a disclosure in the PR description** stating the scope of AI involvement — which parts were AI-generated and what human testing or review was performed prior to submission. PRs that omit this disclosure may be closed without review.

If you are unsure about your work, open a discussion issue to talk it through with the team, or reach out to a maintainer on [Discord](https://discord.gg/RPCS3).

## For maintainers

The check is implemented in [`.github/workflows/pr-gate.yml`](workflows/pr-gate.yml). To let a pull request through without an assigned issue, either add the `Gate: Approved` label, or reopen the pull request yourself, which applies the label automatically. Pull requests closed by the automation carry the `Needs Assigned Issue` label.
