If gbwm ever gets contributors, here's a short memo with the rules:

1. Do not combine multiple unrelated changes in a single commit.
   If your commit touches several independent features or fixes and isn't logically atomic, split it.

2. Use natural-language commit prefixes.
   Examples of valid prefixes:
   - feat, feature, featured — for small useful improvements
   - add, added — for new functionality
   - del, deleted, removed — for removed functionality
   - refactor, refactored — for code improvements that don't change behavior

   Good commit messages look like:
   ```
   featured useful keybindings for laptop users
   XF86 keybindings fix
   added remembering for last focused window
   feat: moved config out of the main file
   ```
