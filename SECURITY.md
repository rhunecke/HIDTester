# Security Policy

## Supported Versions

Only the latest release of HID Tester receives security fixes. Please update to the latest version before reporting a vulnerability.

| Version | Supported |
|---------|-----------|
| Latest release | ✅ Yes |
| Older releases | ❌ No |

## Reporting a Vulnerability

**Please do not open a public GitHub Issue to report a security vulnerability.**

If you discover a security vulnerability, please report it responsibly using one of the following methods:

1. **GitHub Private Vulnerability Reporting** (preferred): Use the [Security Advisories](../../security/advisories/new) feature of this repository to open a private draft advisory. GitHub will notify the maintainer directly.
2. **Email**: If GitHub Private Reporting is unavailable, you can reach the maintainer through the contact information listed on [roberthunecke.com](https://www.roberthunecke.com).

### What to include

Please provide as much of the following information as possible to help us understand and reproduce the issue:

- A clear description of the vulnerability and its potential impact
- Steps to reproduce the issue (proof-of-concept, screenshots, or logs)
- The operating system and version you tested on
- The HID Tester version or commit SHA

### What to expect

- **Acknowledgement**: You will receive an acknowledgement within 72 hours.
- **Assessment**: We will assess the severity and scope of the vulnerability as quickly as possible.
- **Resolution**: A fix will be developed and released. The timeline depends on complexity, but we aim for a patch within 30 days for high-severity issues.
- **Credit**: With your permission, we will credit you in the release notes once the fix is published.

## Scope

This security policy covers the source code of HID Tester itself. Vulnerabilities in third-party dependencies (SDL2, Dear ImGui) should be reported directly to their respective maintainers.

## Out of Scope

- Bugs that do not have a security impact
- Issues caused by misconfigured user environments
- Theoretical vulnerabilities with no practical exploit path
