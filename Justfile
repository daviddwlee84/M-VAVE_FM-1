# Documentation tasks. Run `just` to list them.
default:
    @just --list

# Build the site in strict mode.
docs-build:
    uv run --locked --group docs zensical build --strict

# Build, then verify the generated output (llms.txt, Markdown copies, search).
docs-check: docs-build
    uv run --locked --group docs python scripts/check-docs-site.py

# Live preview. `just docs-serve 8769` picks another port.
docs-serve port="8000" host="127.0.0.1":
    uv run --locked --group docs zensical serve --dev-addr "{{host}}:{{port}}"
