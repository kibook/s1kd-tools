# XQilla XPath interface for s1kd-brexcheck

To set up XQilla for use with s1kd-brexcheck, do the following:

1. Ensure [Xerces-C](https://xerces.apache.org/xerces-c/) and [XQilla](http://xqilla.sourceforge.net) are installed.

   - **Linux:** Both are available in major package managers.
   - **Cygwin:** Xerces-C is available in the standard package manager. XQilla binaries can be download [here](https://khzae.net/1/s1000d/xml/xqilla).

2. Enable XQilla when building:
   ```
   $ make BREXCHECK_XPATH_ENGINE=XQILLA
   ```
