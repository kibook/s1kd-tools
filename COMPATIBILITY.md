General
=======

This document summarizes the compatibility between the s1kd-tools and
each issue of the S1000D specification for which support is planned.

\[empty\]  
Support is planned but not yet implemented.

X  
Support is implemented.

/  
Support is partially implemented.

\~  
Support is not planned. Usually this is because older issues of the
specification did not cover the function of the tool.

> **Note**
>
> Although a tool may not directly support an issue of S1000D, it may
> still be possible to use with that issue.
>
> For example, the s1kd-brexcheck tool is said not to support Issue 2.0
> or Issue 2.1, because the BREX data module schema was not introduced
> until Issue 2.2. However, an Issue 2.2 or greater BREX data module can
> still be used to check Issue 2.0 or Issue 2.1 CSDB objects.

| Tool            | 5.0 | 4.2 | 4.1 | 4.0 | 3.0 | 2.3 | 2.2 | 2.1 | 2.0 |
|-----------------|-----|-----|-----|-----|-----|-----|-----|-----|-----|
| s1kd-acronyms   | X   | X   | X   | X   | X   | X   | X   | X   | X   |
| s1kd-addicn     | X   | X   | X   | X   | X   | X   | X   | X   | X   |
| s1kd-appcheck   | X   | X   | X   | X   | X   | \~  | \~  | \~  | \~  |
| s1kd-aspp       | X   | X   | X   | X   | X   | \~  | \~  | \~  | \~  |
| s1kd-brexcheck  | X   | X   | X   | X   | X   | X   | X   | \~  | \~  |
| s1kd-defaults   | X   | X   | X   | X   | X   | X   | X   | X   | X   |
| s1kd-dmrl       | X   | X   | X   | X   | X   | X   | X   | X   | X   |
| s1kd-flatten    | X   | X   | X   | X   | X   | X   | X   | X   | X   |
| s1kd-fmgen      | X   | X   | X   | \~  | \~  | \~  | \~  | \~  | \~  |
| s1kd-icncatalog | X   | X   | X   | X   | X   | X   | X   | X   | X   |
| s1kd-index      | X   | X   | X   | X   | X   | X   | X   | X   | X   |
| s1kd-instance   | X   | X   | X   | X   | /   | \~  | \~  | \~  | \~  |
| s1kd-ls         | X   | X   | X   | X   | X   | X   | X   | X   | X   |
| s1kd-metadata   | X   | X   | X   | X   | X   | X   | X   | X   | X   |
| s1kd-mvref      | X   | X   | X   | X   | X   | X   | X   | X   | X   |
| s1kd-neutralize | X   | X   | X   | X   | X   | X   | X   | X   | X   |
| s1kd-newcom     | X   | X   | X   | X   | X   | X   | X   | X   | X   |
| s1kd-newddn     | X   | X   | X   | X   | X   | X   | X   | X   | X   |
| s1kd-newdm      | X   | X   | X   | X   | X   | X   | X   | X   | X   |
| s1kd-newdml     | X   | X   | X   | X   | X   | X   | X   | X   | X   |
| s1kd-newimf     | X   | X   | \~  | \~  | \~  | \~  | \~  | \~  | \~  |
| s1kd-newpm      | X   | X   | X   | X   | X   | X   | X   | X   | X   |
| s1kd-newsmc     | X   | X   | X   | \~  | \~  | \~  | \~  | \~  | \~  |
| s1kd-newupf     | X   | X   | X   | \~  | \~  | \~  | \~  | \~  | \~  |
| s1kd-ref        | X   | X   | X   | X   | X   | X   | X   | X   | X   |
| s1kd-refs       | X   | X   | X   | X   | X   | X   | X   | X   | X   |
| s1kd-repcheck   | X   | X   | X   | X   | X   | X   | \~  | \~  | \~  |
| s1kd-sns        | X   | X   | X   | X   | \~  | \~  | \~  | \~  | \~  |
| s1kd-syncrefs   | X   | X   | X   | X   | X   | X   | X   | X   | X   |
| s1kd-uom        | X   | X   | X   | X   | X   | X   | \~  | \~  | \~  |
| s1kd-upissue    | X   | X   | X   | X   | X   | X   | X   | X   | X   |
| s1kd-validate   | X   | X   | X   | X   | X   | X   | X   | X   | X   |
