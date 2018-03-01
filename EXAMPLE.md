General
=======

This document provides examples of the usage of the **s1kd-tools**.

The sample commands have been written as they would be used on a Linux or other Unix-like system, but should work more-or-less the same on most operating systems. OS-specific commands used in examples (e.g., `mkdir`) may need to be adapted.

Initial setup
=============

This first step is to create a folder for the new S1000D project. Example:

    $ mkdir myproject
    $ cd myproject

After that, you should create two files: `defaults` and `dmtypes`.

`defaults` file
---------------

The `defaults` file is used by all of the s1kd-new\* tools. It provides default values for various S1000D metadata. The `defaults` file can be written in either a simple text format or an XML format.

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

`dmtypes` file
--------------

The `dmtypes` file is used by the **s1kd-newdm** tool. It contains a list of information codes and associated info names and schemas to be used when creating new data modules. Like the `defaults` file, it can be written using either the simple text format or XML format.

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

The s1kd-newdm tool contains a default set of information code definitions. This can be used to create a default `dmtypes` file by use of the `-.` (simple text format) or `-,` (XML) options:

`$ s1kd-newdm -, > dmtypes`

The generated `dmtypes` file can then be customized to fit your project.

Creating the DMRL and populating the CSDB
=========================================

The next step is to prepare the DMRL for the project. The DMRL will contain a list of all the CSDB objects initially required by your project, and can be used to automatically populate your CSDB.

If you do not already have a DMRL, the **s1kd-newdml** tool can be used to create a new one:

    $ s1kd-newdml -# MYPRJ-NCAGE-C-2017-00001

This would create the file `DML-MYPRJ-NCAGE-C-2017-00001_000-01.XML` in your CSDB folder.

Once the DMRL is prepared, the **s1kd-dmrl** tool can be used to automatically populate the CSDB based on the CSDB objects listed in the DMRL:

    $ s1kd-dmrl DML-MYPRJ-NCAGE-C-2017-00001_000-01.XML

Information not included in the DMRL entry for a CSDB object is pulled from the `defaults` file (and the `dmtypes` file for data modules).

Creating CSDB objects on-the-fly
--------------------------------

Data modules and other CSDB objects can also be created in an "on-the-fly" manner, without the use of a DMRL, by invoking the s1kd-new\* set of tools directly, as with s1kd-newdml above. For example, to create a new data module:

    $ s1kd-newdm -# MYPRJ-A-00-00-00-00A-040A-D

This would create the file `DMC-MYPRJ-A-00-00-00-00A-040A-D_000-01_EN-CA.XML` in your CSDB folder.

Each of the s1kd-new\* tools has various options for setting specific metadata, and information not included as arguments to these commands is pulled from the `defaults` and `dmtypes` files.

Data module workflow
====================

Data modules are put through the general S1000D workflow with the **s1kd-upissue** tool. Whenever a data module will be changed, the s1kd-upissue tool should first be used to indicate the forthcoming change, creating the next inwork issue of the data module.

Inwork data modules
-------------------

When a data module is in the inwork state, the s1kd-upissue tool is called without any additional arguments:

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

-   Its references to other CSDB objects are valid

-   The actual narrative (content) is correct

The first two points can be verified with the **s1kd-validate** tool. This tool will indicate any problems with the data module in terms of XML syntax and its correctness regarding its S1000D schema.

The third point can be verified using the **s1kd-brexcheck** tool. This tool will indicate any places where a data module violates computable business rules.

The fourth point can be checked using the **s1kd-checkrefs** tool. This tool checks the references within a data module and highlights any references which cannot be resolved.

In contrast to the first four points, which can be verified automatically, the last point is generally not an automatic process, and involves quality assurance testing by a human. That a data module has been first and second QA tested can be indicated with the s1kd-upissue tool:

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

The data module is once again upissued to the next official issue, and the issue type is set to one of the "`rinstate-..."` types.
