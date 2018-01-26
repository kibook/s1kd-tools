General
=======

This document contains an alphabetic index of all valid `defaults` file identifiers, and a description of the values they may be assigned. It also contains examples of a `defaults` file using all identifiers in both the XML format and simple text format.

Alphabetic index
================

| Identifier                      | Value description                        |
|---------------------------------|------------------------------------------|
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
| `inWork`                        | 2 digits                                 |
| `issueNumber`                   | 3 digits                                 |
| `issue`                         | S1000D issue number (4.2, 4.1, 4.0, 3.0) |
| `itemLocationCode`              | A, B, C, D, or T                         |
| `languageIsoCode`               | 2 to 3 character ISO language code       |
| `learnCode`                     | 3 alphanumeric characters                |
| `learnEventCode`                | A, B, C, D, or E                         |
| `modelIdentCode`                | 1 to 14 alphanumeric characters          |
| `originatorCode`                | 5-character NCAGE code                   |
| `originator`                    | string                                   |
| `pmIssuer`                      | 5-character NCAGE code                   |
| `pmNumber`                      | 5 alphanumeric characters                |
| `pmVolume`                      | 2 digits                                 |
| `receiverCity`                  | string                                   |
| `receiverCountry`               | string                                   |
| `receiverIdent`                 | 5-character NCAGE code                   |
| `remarks`                       | string                                   |
| `responsiblePartnerCompanyCode` | 5-character NCAGE code                   |
| `responsiblePartnerCompany`     | string                                   |
| `schema`                        | URI                                      |
| `securityClassification`        | 2 digits                                 |
| `senderIdent`                   | 5-character NCAGE code                   |
| `seqNumber`                     | 00001-99999                              |
| `sns`                           | Filename of BREX data module             |
| `subSubSystemCode`              | 1 alphanumeric character                 |
| `subSystemCode`                 | 1 alphanumeric character                 |
| `systemCode`                    | 2 to 3 alphanumeric characters           |
| `systemDiffCode`                | 1 to 4 alphanumeric characters           |
| `techName`                      | string                                   |
| `yearOfDataIssue`               | 4 digits                                 |

Example - XML format
====================

    <?xml version="1.0"?>
    <defaults>
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
      <default ident="infoCode" value="040"/>
      <default ident="infoName" value="Description"/>
      <default ident="inWork" value="01"/>
      <default ident="issueNumber" value="000"/>
      <default ident="issue" value="4.2"/>
      <default ident="itemLocationCode" value="D"/>
      <default ident="languageIsoCode" value="en"/>
      <default ident="learnCode" value="H10"/>
      <default ident="learnEventCode" value="A"/>
      <default ident="modelIdentCode" value="MYPRJ"/>
      <default ident="originatorCode" value="12345"/>
      <default ident="originator" value="khzae.net"/>
      <default ident="pmIssuer" value="12345"/>
      <default ident="pmNumber" value="00000"/>
      <default ident="pmVolume" value="00"/>
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
      <default ident="sns" value="DMC-MYPRJ-A-00-00-00-00A-022A-D_EN-CA.XML"/>
      <default ident="subSubSystem" value="0"/>
      <default ident="subSystem" value="0"/>
      <default ident="systemCode" value="00"/>
      <default ident="techName" value="My project"/>
      <default ident="yearOfDataIssue" value="2017"/>
    </defaults>

Example - Simple text format
============================

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
    infoCode                       040
    infoName                       Description
    inWork                         01
    issueNumber                    000
    issue                          4.2
    itemLocationCode               D
    languageIsoCode                en
    learnCode                      H10
    learnEventCode                 A
    modelIdentCode                 MYPRJ
    originatorCode                 12345
    originator                     khzae.net
    pmIssuer                       12345
    pmNumber                       00000
    pmVolume                       00
    receiverCity                   Toronto
    receiverCountry                Canada
    receiverIdent                  12345
    remarks                        Comments on a data module
    responsiblePartnerCompanyCode  12345
    responsiblePartnerCompany      khzae.net
    schema                         descript.xsd
    securityClassification         01
    senderIdent                    12345
    seqNumber                      00001
    sns                            DMC-MYPRJ-A-00-00-00-00A-022A-D_EN-CA.XML
    subSubSystem                   0
    subSystem                      0
    systemCode                     00
    techName                       My project
    yearOfDataIssue                2017
