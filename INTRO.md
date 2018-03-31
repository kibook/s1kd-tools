NAME
====

s1kd-tools - Tools for S1000D data

DESCRIPTION
===========

S1000D
------

**S1000D** is "an international specification for the procurement and production of technical publications", part of the S-Series of ILS specifications. The main focus of S1000D is the breakdown and classification of documents in to individual components, called "data modules", which can be re-used in multiple publications. These data modules are typically authored using a set of provided XML schemas, allowing them to be automatically managed in a Common Source Database (CSDB) and validated against a defined set of project "business rules".

s1kd-tools
----------

The **s1kd-tools** are a set of small tools for creating and manipulating S1000D data. Together, they form a "suite" of tools for producing and maintaining S1000D publications within a CSDB. They may also be used individually, to support other S1000D "suites" (usually from an S1000D "vendor") already in use by a project.

CSDB
----

Common Source Databases can be implemented in any number of ways. For the purposes of the s1kd-tools, the CSDB is simply a directory within a filesystem. Use of the "File-based transfer" file naming conventions in Chap 7 of the S1000D specification are recommended, and most of the tools will use these conventions when creating or listing CSDB objects represented by files. In order to use these tools in conjuction with other implementations of CSDBs, a project can make use of "transfer packages" also described in Chap 7 to facilitate interchange between the two kinds of CSDB.

Relationship to the S1000D process
----------------------------------

The s1kd-tools can support multiple parts of the basic S1000D process:

1.  **Generation:** The generation of new CSDB objects is supported by the **s1kd-dmrl** tool and the **s1kd-new\*** set of tools. These provide two methods of creating objects, either using a data management requirements list (DMRL) or a more on-the-fly approach using the s1kd-new\* tools directly.

    The **s1kd-defaults** tool is used to manage the files which contain default metadata for new CSDB objects.

2.  **Authoring:** These tools support the authoring process.

    The **s1kd-addicn** tool creates the notation and entity elements to reference an ICN in a data module.

    The **s1kd-ls** tool lists data modules within a directory.

    The **s1kd-metadata** tool lists and edits S1000D metadata on CSDB objects.

    The **s1kd-ref** tool can be used to quickly insert references to other CSDB objects.

    The **s1kd-transform** tool applies XSLT transformations to CSDB objects.

    The **s1kd-upissue** tool moves CSDB objects through the standard S1000D workflow, between "inwork" (draft) and "official" states.

3.  **Validation:** These tools all validate different aspects of CSDB objects.

    The **s1kd-validate** tool validates CSDB objects according to their S1000D schema and general correctness as XML documents.

    The **s1kd-brexcheck** tool validates CSDB objects against a business rules exchange (BREX) data module, which contains the project-defined computable business rules.

    The **s1kd-refls** tool lists references in a CSDB object to generate a list of dependencies on other CSDB objects.

    The **s1kd-checkrefs** tool validates references between CSDB objects.

4.  **Publication:** These tools support the production of publications from a CSDB.

    The **s1kd-acronyms** tool can automatically mark up acronyms within data modules, and can also generate lists of acronyms marked up within data modules.

    The **s1kd-aspp** tool preprocesses applicability statements in a data module, generating display text and "presentation" applicability statements.

    The **s1kd-flatten** tool flattens a publication module and referenced data modules in to a single "deliverable" file for a publishing system.

    The **s1kd-index** tool flags index keywords in a data module based on a user-defined list.

    The **s1kd-instance** tool produces "instances" of CSDB objects using applicability filtering and/or common information repositories (CIRs).

    The **s1kd-neutralize** tool generates IETP neutral metadata for CSDB objects.

    The **s1kd-syncrefs** tool generates the References table within data modules.

SEE ALSO
========

S1000D website: http://www.s1000d.org

s1kd-acronyms(1), s1kd-aspp(1), s1kd-addicn(1), s1kd-brexcheck(1), s1kd-checkrefs(1), s1kd-ls(1), s1kd-defaults(1), s1kd-dmrl(1), s1kd-flatten(1), s1kd-index(1), s1kd-instance(1), s1kd-metadata(1), s1kd-neutralize(1), s1kd-newcom(1), s1kd-newddn(1), s1kd-newdm(1), s1kd-newdml(1), s1kd-newimf(1), s1kd-newpm(1), s1kd-newupf(1), s1kd-ref(1), s1kd-syncrefs(1), s1kd-transform(1), s1kd-upissue(1), s1kd-validate(1)
