General
=======

This document contains an alphabetic index of all valid `.defaults` file
identifiers, and a description of the values they may be assigned. It
also contains examples of a `.defaults` file using all identifiers in
both the XML format and simple text format.

Alphabetic index
================

| Identifier                      | Value description                        |
|---------------------------------|------------------------------------------|
| `act`                           | Data module code of ACT data module      |
| `assyCode`                      | 2 to 4 alphanumeric characters           |
| `authorization`                 | string                                   |
| `brex`                          | Data module code of BREX data module     |
| `city`                          | string (Sender city)                     |
| `commentPriorityCode`           | cp01-cp99                                |
| `commentType`                   | Q, I, or R                               |
| `countryIsoCode`                | ISO 2-character country code             |
| `country`                       | string (Sender country)                  |
| `disassyCodeVariant`            | 1 to 3 alphanumeric characters           |
| `disassyCode`                   | 2 alphanumeric characters                |
| `dmlType`                       | C, P, or S                               |
| `infoCodeVariant`               | 1 alphanumeric character                 |
| `infoCode`                      | 3 alphanumeric characters                |
| `infoName`                      | string                                   |
| `infoNameVariant`               | string                                   |
| `inWork`                        | 2 digits                                 |
| `issueNumber`                   | 3 digits                                 |
| `issueType`                     | S1000D issue type (new, changed, ...)    |
| `issue`                         | S1000D issue number (5.0, 4.2, 4.1, ...) |
| `itemLocationCode`              | A, B, C, D, or T                         |
| `languageIsoCode`               | 2 to 3 character ISO language code       |
| `learnCode`                     | 3 alphanumeric characters                |
| `learnEventCode`                | A, B, C, D, or E                         |
| `maintainedSns`                 | string                                   |
| `modelIdentCode`                | 1 to 14 alphanumeric characters          |
| `originatorCode`                | 5-character NCAGE code                   |
| `originator`                    | string                                   |
| `pmIssuer`                      | 5-character NCAGE code                   |
| `pmNumber`                      | 5 alphanumeric characters                |
| `pmVolume`                      | 2 digits                                 |
| `receiver`                      | string                                   |
| `receiverCity`                  | string                                   |
| `receiverCountry`               | string                                   |
| `receiverIdent`                 | 5-character NCAGE code                   |
| `remarks`                       | string                                   |
| `responsiblePartnerCompanyCode` | 5-character NCAGE code                   |
| `responsiblePartnerCompany`     | string                                   |
| `schema`                        | URI                                      |
| `scormContentPackageIssuer`     | 5-character NCAGE code                   |
| `scormContentPackageNumber`     | 5 alphanumeric characters                |
| `scormContentPackageVolume`     | 2 digits                                 |
| `securityClassification`        | 2 digits                                 |
| `sender`                        | string                                   |
| `senderIdent`                   | 5-character NCAGE code                   |
| `seqNumber`                     | 00001-99999                              |
| `skillLevelCode`                | sk01-sk99                                |
| `sns`                           | Data module code of BREX data module     |
| `snsLevels`                     | 1 thru 4                                 |
| `subSubSystemCode`              | 1 alphanumeric character                 |
| `subSystemCode`                 | 1 alphanumeric character                 |
| `systemCode`                    | 2 to 3 alphanumeric characters           |
| `systemDiffCode`                | 1 to 4 alphanumeric characters           |
| `techName`                      | string                                   |
| `templates`                     | Path to custom XML templates directory   |
| `yearOfDataIssue`               | 4 digits                                 |

Example - XML format
====================

    <?xml version="1.0"?>
    <defaults>
    <default ident="act" value="MYPRJ-A-00-00-00-00A-00WA-D"/>
    <default ident="assyCode" value="00"/>
    <default ident="authorization" value="khzae.net"/>
    <default ident="brex" value="MYPRJ-A-00-00-00-00A-022A-D"/>
    <default ident="city" value="Toronto"/>
    <default ident="commentPriorityCode" value="cp01"/>
    <default ident="commentType" value="Q"/>
    <default ident="countryIsoCode" value="CA"/>
    <default ident="country" value="Canada"/>
    <default ident="disassyCodeVariant" value="A"/>
    <default ident="disassyCode" value="00"/>
    <default ident="dmlType" value="C"/>
    <default ident="infoCodeVariant" value="A"/>
    <default ident="infoCode" value="258"/>
    <default ident="infoName" value="Other procedure to clean"/>
    <default ident="infoNameVariant" value="Clean with water"/>
    <default ident="inWork" value="01"/>
    <default ident="issueNumber" value="000"/>
    <default ident="issueType" value="new"/>
    <default ident="issue" value="5.0"/>
    <default ident="itemLocationCode" value="D"/>
    <default ident="languageIsoCode" value="en"/>
    <default ident="learnCode" value="H10"/>
    <default ident="learnEventCode" value="A"/>
    <default ident="maintainedSns" value="General sea vehicles"/>
    <default ident="modelIdentCode" value="MYPRJ"/>
    <default ident="omitIssueInfo" value="true"/>
    <default ident="originatorCode" value="12345"/>
    <default ident="originator" value="khzae.net"/>
    <default ident="pmIssuer" value="12345"/>
    <default ident="pmNumber" value="00000"/>
    <default ident="pmVolume" value="00"/>
    <default ident="receiver" value="khzae.net"/>
    <default ident="receiverCity" value="Toronto"/>
    <default ident="receiverCountry" value="Canada"/>
    <default ident="receiverIdent" value="12345"/>
    <default ident="remarks" value="Comments on a data module"/>
    <default ident="responsiblePartnerCompanyCode" value="12345"/>
    <default ident="responsiblePartnerCompany" value="khzae.net"/>
    <default ident="schema" value="descript.xsd"/>
    <default ident="securityClassification" value="01"/>
    <default ident="senderIdent" value="12345"/>
    <default ident="seqNumber" value="00001"/>
    <default ident="skillLevelCode" value="sk01"/>
    <default ident="sns" value="MYPRJ-A-00-00-00-00A-022A-D"/>
    <default ident="snsLevels" value="1"/>
    <default ident="subSubSystem" value="0"/>
    <default ident="subSystem" value="0"/>
    <default ident="systemCode" value="00"/>
    <default ident="techName" value="My project"/>
    <default ident="templates" value="/usr/share/s1kd-tools/templ"/>
    <default ident="yearOfDataIssue" value="2017"/>
    </defaults>

Example - Simple text format
============================

    act                            MYPRJ-A-00-00-00-00A-00WA-D
    assyCode                       00
    authorization                  khzae.net
    brex                           MYPRJ-A-00-00-00-00A-022A-D
    city                           Toronto
    commentPriorityCode            cp01
    commentType                    Q
    countryIsoCode                 CA
    country                        Canada
    disassyCodeVariant             A
    disassyCode                    00
    dmlType                        C
    infoCodeVariant                A
    infoCode                       258
    infoName                       Other procedure to clean
    infoNameVariant                Clean with water
    inWork                         01
    issueNumber                    000
    issueType                      new
    issue                          5.0
    itemLocationCode               D
    languageIsoCode                en
    learnCode                      H10
    learnEventCode                 A
    maintainedSns                  General sea vehicles
    modelIdentCode                 MYPRJ
    omitIssueInfo                  true
    originatorCode                 12345
    originator                     khzae.net
    pmIssuer                       12345
    pmNumber                       00000
    pmVolume                       00
    receiver                       khzae.net
    receiverCity                   Toronto
    receiverCountry                Canada
    receiverIdent                  12345
    remarks                        Comments on a data module
    responsiblePartnerCompanyCode  12345
    responsiblePartnerCompany      khzae.net
    schema                         descript.xsd
    securityClassification         01
    sender                         khzae.net
    senderIdent                    12345
    seqNumber                      00001
    skillLevelCode                 sk01
    sns                            MYPRJ-A-00-00-00-00A-022A-D
    snsLevels                      1
    subSubSystem                   0
    subSystem                      0
    systemCode                     00
    techName                       My project
    templates                      /usr/share/s1kd-tools/templ
    yearOfDataIssue                2017
