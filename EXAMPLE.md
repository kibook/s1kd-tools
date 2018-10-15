-   [General](#general)
-   [Initial setup](#initial-setup)
    -   [`.defaults` file](#defaults-file)
    -   [`.dmtypes` file](#dmtypes-file)
-   [Creating the DMRL and populating the CSDB](#creating-the-dmrl-and-populating-the-csdb)
    -   [Adding DMRL entries](#adding-dmrl-entries)
    -   [Populating the CSDB from the DMRL](#populating-the-csdb-from-the-dmrl)
    -   [Creating CSDB objects on-the-fly](#creating-csdb-objects-on-the-fly)
-   [Data module workflow](#data-module-workflow)
    -   [Inwork data modules](#inwork-data-modules)
    -   [Making data modules official](#making-data-modules-official)
        -   [Validating against the schema](#validating-against-the-schema)
        -   [Validating against a BREX data module](#validating-against-a-brex-data-module)
        -   [Quality assurance verification](#quality-assurance-verification)
    -   [Changes to official data modules](#changes-to-official-data-modules)
    -   [Deleting data modules](#deleting-data-modules)
-   [Building publications](#building-publications)
    -   [Publication module content](#publication-module-content)
    -   [Creating a customized publication](#creating-a-customized-publication)
-   [Use with other version control systems](#use-with-other-version-control-systems)

General
=======

This document provides examples of the usage of the **s1kd-tools**.

The sample commands have been written as they would be used on a Linux or other Unix-like system, but should work more-or-less the same on most operating systems. OS-specific commands used in examples (e.g., `mkdir`) may need to be adapted.

![Example - Authoring with Vim + MuPDF](doc/ICN-S1KDTOOLS-A-000000-A-KHZAE-00002-A-001-01.PNG)

Initial setup
=============

This first step is to create a folder for the new S1000D project. Example:

    $ mkdir myproject
    $ cd myproject

After that, you should create two files: `.defaults` and `.dmtypes`. These files can be created automatically using the **s1kd-defaults** tool to initialize the new CSDB:

    $ s1kd-defaults -i

Afterwards, these files can be edited to customize them for your project. More information on the contents of these files is provided below.

> **Note**
>
> If the tools are run in a directory that does not have these configuration files, they will search for them in the parent directories to find the top of the CSDB directory tree.

`.defaults` file
----------------

The `.defaults` file is used by all of the s1kd-new\* tools. It provides default values for various S1000D metadata. The `.defaults` file can be written in either a simple text format or an XML format.

**Example of simple text format:**

    languageIsoCode            en
    countryIsoCode             CA
    responsiblePartnerCompany  khzae.net
    originator                 khzae.net
    brex                       MYPRJ-A-00-00-00-00A-022A-D
    techName                   My project

**Example of XML format:**

    <?xml version="1.0"?>
    <defaults>
    <default ident="languageIsoCode" value="en"/>
    <default ident="countryIsoCode" value="CA"/>
    <default ident="responsiblePartnerCompany" value="khzae.net"/>
    <default ident="originator" value="khzae.net"/>
    <default ident="brex" value="MYPRJ-A-00-00-00-00A-022A-D"/>
    <default ident="techName" value="My project"/>
    </defaults>

`.dmtypes` file
---------------

The `.dmtypes` file is used by the **s1kd-newdm** tool. It contains a list of information codes and associated info names and schemas to be used when creating new data modules. Like the `.defaults` file, it can be written using either the simple text format or XML format.

**Example of simple text format:**

    009  frontmatter  Table of contents
    022  brex         Business rules exchange
    040  descript     Description
    130  proced       Normal operation

**Example of XML format:**

    <?xml version="1.0"?>
    <dmtypes>
    <type infoCode="009" infoName="Table of contents"
    schema="frontmatter"/>
    <type infoCode="022" infoName="Business rules exchange"
    schema="brex"/>
    <type infoCode="040" infoName="Description"
    schema="descript"/>
    <type infoCode="130" infoName="Normal operation"
    schema="proced"/>
    </dmtypes>

The s1kd-newdm tool contains a default set of information code definitions. This can be used to create a default `.dmtypes` file by use of the `-.` (simple text format) or `-,` (XML) options:

`$ s1kd-newdm -, > .dmtypes`

The generated `.dmtypes` file can then be customized to fit your project.

Creating the DMRL and populating the CSDB
=========================================

The next step is to prepare the Data Management Requirements List (DMRL) for the project. The DMRL will contain a list of all the CSDB objects initially required by your project, and can be used to automatically populate your CSDB.

If you do not already have a DMRL, the **s1kd-newdml** tool can be used to create a new one:

    $ s1kd-newdml -# MYPRJ-NCAGE-C-2017-00001

This would create the file `DML-MYPRJ-NCAGE-C-2017-00001_000-01.XML` in your CSDB folder.

Adding DMRL entries
-------------------

Each entry in the DMRL describes a data module that is planned to be created, giving the data module code, title, security classification and responsible entity:

    <dmlContent>
    <dmlEntry>
    <dmRef>
    <dmRefIdent>
    <dmCode modelIdentCode="MYPRJ" systemDiffCode="A" systemCode="00"
    subSystemCode="0" subSubSystemCode="0" assyCode="00" 
    disassyCode="00" disassyCodeVariant="A" infoCode="040"
    infoCodeVariant="A" itemLocationCode="D"/>
    </dmRefIdent>
    <dmRefAddressItems>
    <dmTitle>
    <techName>My project</techName>
    <infoName>Description</infoName>
    </dmTitle>
    </dmRefAddressItems>
    </dmRef>
    <security securityClassification="01"/>
    <responsiblePartnerCompany>
    <enterpriseName>khzae.net</enterpriseName>
    </responsiblePartnerCompany>
    </dmlEntry>
    ...
    </dmlContent>

The XML for the `dmRef` of each entry can be quickly generated using the **s1kd-ref** tool:

    $ s1kd-ref DMC-MYPRJ-A-00-00-00-00A-040A-D

Populating the CSDB from the DMRL
---------------------------------

Once the DMRL is prepared, the **s1kd-dmrl** tool can be used to automatically populate the CSDB based on the CSDB objects listed in the DMRL:

    $ s1kd-dmrl DML-MYPRJ-NCAGE-C-2017-00001_000-01.XML

Information not included in the DMRL entry for a CSDB object is pulled from the `.defaults` file (and the `.dmtypes` file for data modules).

The DMRL should be updated throughout the lifecycle of a project. When new entries are added, simply use the **s1kd-dmrl** tool again to create the newly added data modules. Already existing data modules will not be overwritten, unless the -f option is specified. The -q option will suppress those messages indicating that a data module that already exists will not be overwritten:

    $ s1kd-dmrl -q DML-MYPRJ-NCAGE-C-2017-00001_000-02.XML

Creating CSDB objects on-the-fly
--------------------------------

Data modules and other CSDB objects can also be created in an "on-the-fly" manner, without the use of a DMRL, by invoking the s1kd-new\* set of tools directly, as with s1kd-newdml above. For example, to create a new data module:

    $ s1kd-newdm -# MYPRJ-A-00-00-00-00A-040A-D

This would create the file `DMC-MYPRJ-A-00-00-00-00A-040A-D_000-01_EN-CA.XML` in your CSDB folder.

Each of the s1kd-new\* tools has various options for setting specific metadata, and information not included as arguments to these commands is pulled from the `.defaults` and `.dmtypes` files.

Data module workflow
====================

Data modules are put through the general S1000D workflow with the **s1kd-upissue** tool. Whenever a data module will be changed, the s1kd-upissue tool should first be used to indicate the forthcoming change, creating the next inwork issue of the data module.

Inwork data modules
-------------------

To increment the inwork issue of a data module, the s1kd-upissue tool is called without any additional options:

    $ s1kd-upissue DMC-MYPRJ-A-00-00-00-00A-040A-D_000-01_EN-CA.XML

Assuming this data module was just created, it would be incremented from initial inwork issue 000-01 to initial inwork issue 000-02. After upissuing, make the changes. For example:

**`DMC-MYPRJ-A-00-00-00-00A-040A-D_000-01_EN-CA.XML`:**

    <content>
    <description>
    <levelledPara>
    <title>General</title>
    <para>This is my project.</para>
    </levelledPara>
    </description>
    </content>

**`DMC-MYPRJ-A-00-00-00-00A-040A-D_000-02_EN-CA.XML`:**

    <content>
    <description>
    <levelledPara>
    <title>General</title>
    <para>This is my project.</para>
    <para>My project is maintained using S1000D.</para>
    </levelledPara>
    </description>
    </content>

Making data modules official
----------------------------

Before a data module can be made official, it must be validated. This means:

-   It is a valid XML file

-   It is valid according to the relevant S1000D schema

-   It is valid according to the relevant business rules

-   The actual narrative (content) is correct

### Validating against the schema

The first two points can be verified with the **s1kd-validate** tool. This tool will indicate any problems with the data module in terms of XML syntax and its correctness regarding its S1000D schema:

    $ s1kd-validate DMC-MYPRJ-A-00-00-00-00A-040A-D_000-03_EN-CA.XML

### Validating against a BREX data module

The third point can be verified using the **s1kd-brexcheck** tool. This tool will indicate any places where a data module violates computable business rules as specified in a Business Rules Exchange (BREX) data module.

    $ s1kd-brexcheck DMC-MYPRJ-A-00-00-00-00A-040A-D_000-03_EN-CA.XML

The BREX allows a project to customize S1000D, for example, by disallowing certain elements or attributes:

    <structureObjectRule>
    <objectPath allowedObjectFlag="0">//emphasis</objectPath>
    <objectUse>The emphasis element is not allowed.</objectUse>
    </structureObjectRule>

Or by tailoring the allowed values of certain elements or attributes:

    <structureObjectRule>
    <objectPath allowedObjectFlag="2">
    //@securityClassification
    </objectPath>
    <objectUse>
    The security classification must be 01 (Unclassified)
    or 02 (Classified).
    </objectUse>
    <objectValue valueAllowed="01">Unclassified</objectValue>
    <objectValue valueAllowed="02">Classified</objectValue>
    </structureObjectRule>

Each data module references the BREX it should be checked against, and BREX data modules can reference other BREX data modules to create a layered set of business rules, for example, Project-related rules and Organization-related rules.

Unless otherwise specified, data modules will reference the S1000D default BREX, which contains a base set of business rules.

To get started with your project's own business rules, you can create a simple BREX data module based on the current defaults of your CSDB using the -B option of the s1kd-newdm tool:

    $ s1kd-newdm -B# MYPRJ-A-00-00-00-00A-022A-D

This will use the customized `.defaults` and `.dmtypes` files to generate a basic set of business rules.

### Quality assurance verification

In contrast to the first three points, which can be verified automatically, the last point is generally not an automatic process, and involves quality assurance testing by a human. That a data module has been first or second QA tested can be indicated with the s1kd-upissue tool:

    $ s1kd-upissue -1 tabtop -2 ttandoo ...

Once the data module is validated, the s1kd-upissue tool is used to make it official with the `-i` option:

    $ s1kd-upissue -i DMC-MYPRJ-A-00-00-00-00A-040A-D_000-03_EN-CA.XML

Changes to official data modules
--------------------------------

When a change must be made to an official data module (for example, as a result of feedback), the s1kd-upissue tool is used again to bring the data module back to the inwork state:

    $ s1kd-upissue DMC-MYPRJ-A-00-00-00-00A-040A-D_001-00_EN-CA.XML

Changes between official issues of a data module are indicated with reasons for update and change marking. For example:

**`DMC-MYPRJ-A-00-00-00-00A-040A-D_001-00_EN-CA.XML`:**

    <content>
    <description>
    <levelledPara>
    <title>General</title>
    <para>This is my project.</para>
    <para>My project is maintained using S1000D.</para>
    </levelledPara>
    </description>
    </content>

**`DMC-MYPRJ-A-00-00-00-00A-040A-D_001-01_EN-CA.XML`:**

    <dmStatus issueType="changed">
    <!-- ...... -->
    <reasonForUpdate id="rfu-0001">
    <simplePara>Added reference to tools used.</simplePara>
    </reasonForUpdate>
    </dmStatus>
    <!-- ...... -->
    <content>
    <description>
    <levelledPara>
    <title>General</title>
    <para>This is my project.</para>
    <para changeType="modify" changeMark="1"
    reasonForUpdateRefIds="rfu-0001">My project is maintained using
    S1000D and s1kd-tools.</para>
    </levelledPara>
    </description>
    </content>

Reasons for update from the previous official issue are automatically removed when upissuing to the first inwork issue.

Deleting data modules
---------------------

The basic cycle continues until a data module is deleted. "Deleting" a data module is a special case of upissuing:

    $ s1kd-upissue -is deleted ...

The data module is upissued to the next official issue, and it's issue type is set to "`deleted`".

Deleted data modules may be reinstated later in a similar way:

    $ s1kd-upissue -is rinstate-status ...

The data module is once again upissued to the next official issue, and the issue type is set to one of the "`rinstate-x"` types.

Building publications
=====================

S1000D publications are managed by use of publication modules. Like data modules, publication modules may be created as part of the project's DMRL:

    <dmlEntry>
    <pmRef>
    <pmRefIdent>
    <pmCode modelIdentCode="MYPRJ" pmIssuer="12345" pmNumber="00001"
    pmVolume="00"/>
    </pmRefIdent>
    <pmRefAddressItems>
    <pmTitle>My publication</pmTitle>
    </pmRefAddressItems>
    </pmRef>
    <responsiblePartnerCompany>
    <enterpriseName>khzae.net</enterpriseName>
    </responsiblePartnerCompany>
    </dmlEntry>

or "on-the-fly" with the **s1kd-newpm** tool:

    $ s1kd-newpm -# MYPRJ-12345-00001-00

Publication module content
--------------------------

The publication module lays out the hierarchical structure of the data modules in a publication:

    <content>
    <pmEntry>
    <pmEntryTitle>Front matter</pmEntryTitle>
    <dmRef>
    <dmRefIdent>
    <dmCode modelIdentCode="MYPRJ" systemDiffCode="A" systemCode="00"
    subSystemCode="0" subSubSystemCode="0" assyCode="00" disassyCode="00"
    disassyCodeVariant="A" infoCode="001" infoCodeVariant="A"
    itemLocationCode="D"/>
    </dmRefIdent>
    <dmRefAddressItems>
    <dmTitle>
    <techName>My project</techName>
    <infoName>Title page</infoName>
    </dmTitle>
    </dmRefAddressItems>
    </dmRef>
    </pmEntry>
    <pmEntry>
    <pmEntryTitle>General info</pmEntryTitle>
    <dmRef>
    <dmRefIdent>
    <dmCode modelIdentCode="MYPRJ" systemDiffCode="A" systemCode="00"
    subSystemCode="0" subSubSystemCode="0" assyCode="00" disassyCode="00"
    disassyCodeVariant="A" infoCode="040" infoCodeVariant="A"
    itemLocationCode="D"/>
    </dmRefIdent>
    <dmRefAddressItems>
    <dmTitle>
    <techName>My project</techName>
    <infoName>Description</infoName>
    </dmTitle>
    </dmRefAddressItems>
    </dmRef>
    </pmEntry>
    </content>

Creating a customized publication
---------------------------------

The S1000D applicability model and the **s1kd-instance** tool enable the creation of customized publications, which are filtered for a particular customer or product. For example, a data module may contain applicabilty for two versions of a product:

    <para>
    This is some common information about the product.
    </para>
    <para applicRefId="app-versionA">
    This information only applies to version A.
    </para>
    <para applicRefId="app-versionB">
    This information only applies to version B.
    </para>

When you deliver this data module to a customer with Version B, you can exclude information which is not applicable to them by filtering it:

    $ s1kd-instance -s version:prodattr=B <DM>

To filter a whole publication, use the -O option of the s1kd-instance tool to output multiple filtered objects into a directory:

    $ s1kd-instance -s version:prodattr=B -O customerB DMC-*.XML

The newly created `customerB` directory will contain the filtered versions of these data modules.

If your CSDB contains multiple, separate publications, the **s1kd-refls** tool can be used to select only those data modules which apply to a particular publication module:

    $ s1kd-refls -s <PM> | xargs s1kd-instance -s version:prodattr=B -O customerB

The above command will filter the publication module and all included data modules, and output the resulting objects to the `customerB` directory.

Use with other version control systems
======================================

The issue/inwork numbers and S1000D file naming conventions as seen above provide a basic form of version control. In this case, each file represents a single issue of a CSDB object, and multiple files together represent the whole logical object. For example, all of the following files represent different versions of the same object:

-   `DMC-MYPRJ-A-00-00-00-00A-040A-D_000-01_EN-CA.XML`

-   `DMC-MYPRJ-A-00-00-00-00A-040A-D_000-02_EN-CA.XML`

-   `DMC-MYPRJ-A-00-00-00-00A-040A-D_001-00_EN-CA.XML`

However, if you prefer to use an existing version control system such as Git or SVN, it is often more useful for each file to represent a whole object, since these systems typically track changes based on filenames.

The s1kd-tools support an alternate naming convention for this case. Specifying the -N option to certain tools will omit the issue and inwork numbers from filenames of CSDB objects. Taking the s1kd-newdm tool example from above, but adding the -N option as follows:

    $ s1kd-newdm -N# MYPRJ-A-00-00-00-00A-040A-D

would create the file `DMC-MYPRJ-A-00-00-00-00A-040A-D_EN-CA.XML` in your CSDB folder. The s1kd-upissue tool works similarly:

    $ s1kd-upissue -Ni DMC-MYPRJ-A-00-00-00-00A-040A-D_EN-CA.XML

The issue and inwork numbers are updated in the XML metadata, but instead of creating a new file, the original is overwritten. The previous inwork issues are therefore stored as part of the external version control's history, rather than as individual files.
