<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">

  <!-- S1000D 4.2 to 3.0 -->

  <xsl:variable name="schema-prefix">http://www.s1000d.org/S1000D_4-2/xml_schema_flat/</xsl:variable>
  
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="@xsi:noNamespaceSchemaLocation">
    <xsl:attribute name="xsi:noNamespaceSchemaLocation">
      <xsl:text>http://www.s1000d.org/S1000D_3-0/xml_schema_flat/</xsl:text>
      <xsl:value-of select="substring-after(., $schema-prefix)"/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="identAndStatusSection">
    <idstatus>
      <xsl:apply-templates/>
    </idstatus>
  </xsl:template>

  <xsl:template match="dmAddress">
    <dmaddres>
      <xsl:apply-templates/>
    </dmaddres>
  </xsl:template>

  <xsl:template match="dmRef">
    <refdm>
      <xsl:apply-templates select="@*|node()"/>
    </refdm>
  </xsl:template>

  <xsl:template match="dmIdent">
    <dmc>
      <xsl:apply-templates select="dmCode"/>
    </dmc>
    <xsl:apply-templates select="../dmAddressItems/dmTitle"/>
    <xsl:apply-templates select="issueInfo"/>
    <xsl:apply-templates select="../dmAddressItems/issueDate"/>
    <xsl:apply-templates select="language"/>
  </xsl:template>

  <xsl:template match="dmRefIdent">
    <xsl:apply-templates select="dmCode"/>
    <xsl:apply-templates select="../dmAddressItems/dmTitle"/>
    <xsl:apply-templates select="issueInfo"/>
    <xsl:apply-templates select="../dmAddressItems/issueDate"/>
    <xsl:apply-templates select="language"/>
  </xsl:template>

  <xsl:template match="dmCode">
    <avee>
      <xsl:apply-templates select="@*"/>
    </avee>
  </xsl:template>

  <xsl:template match="@modelIdentCode">
    <modelic>
      <xsl:apply-templates/>
    </modelic>
  </xsl:template>

  <xsl:template match="@systemDiffCode">
    <sdc>
      <xsl:apply-templates/>
    </sdc>
  </xsl:template>

  <xsl:template match="@systemCode">
    <chapnum>
      <xsl:apply-templates/>
    </chapnum>
  </xsl:template>

  <xsl:template match="@subSystemCode">
    <section>
      <xsl:apply-templates/>
    </section>
  </xsl:template>

  <xsl:template match="@subSubSystemCode">
    <subsect>
      <xsl:apply-templates/>
    </subsect>
  </xsl:template>

  <xsl:template match="@assyCode">
    <subject>
      <xsl:apply-templates/>
    </subject>
  </xsl:template>

  <xsl:template match="@disassyCode">
    <discode>
      <xsl:apply-templates/>
    </discode>
  </xsl:template>

  <xsl:template match="@disassyCodeVariant">
    <discodev>
      <xsl:apply-templates/>
    </discodev>
  </xsl:template>

  <xsl:template match="@infoCode">
    <incode>
      <xsl:apply-templates/>
    </incode>
  </xsl:template>

  <xsl:template match="@infoCodeVariant">
    <incodev>
      <xsl:apply-templates/>
    </incodev>
  </xsl:template>

  <xsl:template match="@itemLocationCode">
    <itemloc>
      <xsl:apply-templates/>
    </itemloc>
  </xsl:template>

  <xsl:template match="issueInfo">
    <issno>
      <xsl:apply-templates select="@*|node()"/>
      <xsl:apply-templates select="//dmStatus/@issueType"/>
    </issno>
  </xsl:template>

  <xsl:template match="@issueNumber">
    <xsl:attribute name="issno">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="@inWork">
    <xsl:attribute name="inwork">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="@issueType">
    <xsl:attribute name="type">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="dmTitle">
    <dmtitle>
      <xsl:apply-templates select="@*|node()"/>
    </dmtitle>
  </xsl:template>

  <xsl:template match="techName">
    <techname>
      <xsl:apply-templates select="@*|node()"/>
    </techname>
  </xsl:template>

  <xsl:template match="infoName">
    <infoname>
      <xsl:apply-templates select="@*|node()"/>
    </infoname>
  </xsl:template>

  <xsl:template match="issueDate">
    <issdate>
      <xsl:apply-templates select="@*|node()"/>
    </issdate>
  </xsl:template>

  <xsl:template match="brexDmRef">
    <brexref>
      <xsl:apply-templates select="@*|node()"/>
    </brexref>
  </xsl:template>

  <xsl:template match="displayText">
    <displaytext>
      <xsl:apply-templates select="@*|node()"/>
    </displaytext>
  </xsl:template>

  <xsl:template match="@languageIsoCode">
    <xsl:attribute name="language">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="@countryIsoCode">
    <xsl:attribute name="country">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="dmAddressItems"/>

  <xsl:template match="dmStatus">
    <status>
      <xsl:apply-templates select="node()"/>
    </status>
  </xsl:template>

  <xsl:template match="qualityAssurance">
    <qa>
      <xsl:apply-templates select="@*|node()"/>
    </qa>
  </xsl:template>

  <xsl:template match="unverified">
    <unverif/>
  </xsl:template>

  <xsl:template match="@securityClassification">
    <xsl:attribute name="class">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="@commercialClass">
    <xsl:attribute name="commcls">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="responsiblePartnerCompany">
    <rpc>
      <xsl:apply-templates select="enterpriseName"/>
      <xsl:apply-templates select="@enterpriseCode"/>
    </rpc>
  </xsl:template>

  <xsl:template match="originator">
    <orig>
      <xsl:apply-templates select="enterpriseName"/>
      <xsl:apply-templates select="@enterpriseCode"/>
    </orig>
  </xsl:template>

  <xsl:template match="responsiblePartnerCompany/enterpriseName">
    <xsl:attribute name="rpcname">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="originator/enterpriseName">
    <xsl:attribute name="origname">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="@enterpriseCode">
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="description">
    <descript>
      <xsl:apply-templates/>
    </descript>
  </xsl:template>

  <xsl:template match="simplePara">
    <p>
      <xsl:apply-templates select="@*|node()"/>
    </p>
  </xsl:template>

  <xsl:template match="procedure">
    <proced>
      <xsl:apply-templates select="@*|node()"/>
    </proced>
  </xsl:template>

  <xsl:template match="preliminaryRqmts">
    <prelreqs>
      <xsl:apply-templates select="@*|node()"/>
    </prelreqs>
  </xsl:template>

  <xsl:template match="reqCondGroup">
    <reqconds>
      <xsl:apply-templates select="@*|node()"/>
    </reqconds>
  </xsl:template>

  <xsl:template match="noConds">
    <noconds/>
  </xsl:template>

  <xsl:template match="reqSupportEquips">
    <supequip>
      <xsl:apply-templates select="@*|node()"/>
    </supequip>
  </xsl:template>

  <xsl:template match="noSupportEquips">
    <nosupeq/>
  </xsl:template>

  <xsl:template match="reqSupplies">
    <supplies>
      <xsl:apply-templates select="@*|node()"/>
    </supplies>
  </xsl:template>

  <xsl:template match="noSupplies">
    <nosupply/>
  </xsl:template>

  <xsl:template match="reqSpares">
    <spares>
      <xsl:apply-templates select="@*|node()"/>
    </spares>
  </xsl:template>

  <xsl:template match="noSpares">
    <nospares/>
  </xsl:template>

  <xsl:template match="reqSafety">
    <safety>
      <xsl:apply-templates select="@*|node()"/>
    </safety>
  </xsl:template>

  <xsl:template match="noSafety">
    <nosafety/>
  </xsl:template>

  <xsl:template match="mainProcedure">
    <mainfunc>
      <xsl:apply-templates select="@*|node()"/>
    </mainfunc>
  </xsl:template>

  <xsl:template match="closeRqmts">
    <closereqs>
      <xsl:apply-templates select="@*|node()"/>
    </closereqs>
  </xsl:template>

  <xsl:template match="proceduralStep">
    <step1>
      <xsl:apply-templates select="@*|node()"/>
    </step1>
  </xsl:template>

  <xsl:template match="faultIsolation">
    <afi>
      <xsl:apply-templates select="@*|node()"/>
    </afi>
  </xsl:template>

  <xsl:template match="faultIsolationProcedure">
    <afi-proc>
      <xsl:apply-templates select="@*|node()"/>
    </afi-proc>
  </xsl:template>

  <xsl:template match="isolationProcedure">
    <isoproc>
      <xsl:apply-templates select="@*|node()"/>
    </isoproc>
  </xsl:template>

  <xsl:template match="isolationMainProcedure">
    <isolatep>
      <xsl:apply-templates select="@*|node()"/>
    </isolatep>
  </xsl:template>

  <xsl:template match="isolationProcedure/closeRqmts">
    <closetxt/>
  </xsl:template>

  <xsl:template match="isolationProcedureEnd">
    <isoend>
      <xsl:apply-templates select="@*|node()"/>
    </isoend>
  </xsl:template>

  <xsl:template match="snsRules"/>

  <xsl:template match="contextRules">
    <contextrules>
      <xsl:apply-templates select="@*|node()"/>
    </contextrules>
  </xsl:template>

  <xsl:template match="nonContextRules"/>

  <xsl:template match="applicCrossRefTable">
    <act>
      <xsl:apply-templates select="@*|node()"/>
    </act>
  </xsl:template>

  <xsl:template match="condCrossRefTable">
    <cct>
      <xsl:apply-templates select="@*|node()"/>
    </cct>
  </xsl:template>

  <xsl:template match="productCrossRefTable">
    <pct>
      <xsl:apply-templates select="@*|node()"/>
    </pct>
  </xsl:template>

  <xsl:template match="dmSeq">
    <dm-seq>
      <xsl:apply-templates select="@*|node()"/>
    </dm-seq>
  </xsl:template>

  <xsl:template match="dmNode">
    <dm-node>
      <xsl:apply-templates select="@*|node()"/>
    </dm-node>
  </xsl:template>

  <xsl:template match="illustratedPartsCatalog">
    <ipc>
      <xsl:apply-templates select="@*|node()"/>
    </ipc>
  </xsl:template>

  <xsl:template match="catalogSeqNumber">
    <csn>
      <xsl:apply-templates select="@*|node()"/>
    </csn>
  </xsl:template>

  <xsl:template match="@figureNumber">
    <xsl:attribute name="csn">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="@indenture">
    <xsl:attribute name="ind">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="itemSeqNumber">
    <isn>
      <xsl:apply-templates select="@*|node()"/>
    </isn>
  </xsl:template>

  <xsl:template match="@itemSeqNumberValue">
    <xsl:attribute name="isn">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="quantityPerNextHigherAssy">
    <qna>
      <xsl:apply-templates select="@*|node()"/>
    </qna>
  </xsl:template>

  <xsl:template match="partRef">
    <xsl:apply-templates select="@*"/>
  </xsl:template>

  <xsl:template match="@manufacturerCodeValue">
    <mfc>
      <xsl:apply-templates/>
    </mfc>
  </xsl:template>

  <xsl:template match="@partNumberValue">
    <pnr>
      <xsl:apply-templates/>
    </pnr>
  </xsl:template>

  <xsl:template match="pmCode">
    <pmc>
      <xsl:apply-templates select="@*"/>
    </pmc>
  </xsl:template>

  <xsl:template match="@pmIssuer">
    <pmissuer>
      <xsl:apply-templates/>
    </pmissuer>
  </xsl:template>

  <xsl:template match="@pmNumber">
    <pmnumber>
      <xsl:apply-templates/>
    </pmnumber>
  </xsl:template>

  <xsl:template match="@pmVolume">
    <pmvolume>
      <xsl:apply-templates/>
    </pmvolume>
  </xsl:template>

  <xsl:template match="pmAddress">
    <pmaddres>
      <xsl:apply-templates select="pmIdent/pmCode"/>
      <xsl:apply-templates select="pmAddressItems/pmTitle"/>
      <xsl:apply-templates select="pmIdent/issueInfo"/>
      <xsl:apply-templates select="pmAddressItems/issueDate"/>
      <xsl:apply-templates select="pmIdent/language"/>
    </pmaddres>
  </xsl:template>

  <xsl:template match="pmIdent|pmRefIdent">
    <xsl:apply-templates select="pmCode"/>
  </xsl:template>

  <xsl:template match="pmTitle">
    <pmtitle>
      <xsl:apply-templates/>
    </pmtitle>
  </xsl:template>

  <xsl:template match="pmStatus">
    <pmstatus>
      <xsl:apply-templates select="security"/>
      <xsl:apply-templates select="responsiblePartnerCompany"/>
      <xsl:apply-templates select="originator"/>
      <xsl:apply-templates select="applicCrossRefTableRef"/>
      <xsl:if test="not(applicCrossRefTableRef)">
        <actref>
          <refdm/>
        </actref>
      </xsl:if>
      <xsl:apply-templates select="applic"/>
      <xsl:apply-templates select="qualityAssurance"/>
    </pmstatus>
  </xsl:template>

  <xsl:template match="pmEntry">
    <pmentry>
      <xsl:apply-templates select="@*|node()"/>
    </pmentry>
  </xsl:template>

  <xsl:template match="pmEntryTitle">
    <title>
      <xsl:apply-templates select="@*|node()"/>
    </title>
  </xsl:template>

  <xsl:template match="commentCode">
    <ccode>
      <xsl:apply-templates select="@*"/>
    </ccode>
  </xsl:template>

  <xsl:template match="@senderIdent">
    <sendid>
      <xsl:apply-templates/>
    </sendid>
  </xsl:template>

  <xsl:template match="@yearOfDataIssue">
    <diyear>
      <xsl:apply-templates/>
    </diyear>
  </xsl:template>

  <xsl:template match="@seqNumber">
    <seqnum>
      <xsl:apply-templates/>
    </seqnum>
  </xsl:template>

  <xsl:template match="@commentType">
    <ctype type="{.}"/>
  </xsl:template>

  <xsl:template match="commentAddress">
    <xsl:apply-templates select="commentIdent/commentCode"/>
    <xsl:apply-templates select="commentAddressItems/issueDate"/>
    <xsl:apply-templates select="commentIdent/language"/>
    <xsl:apply-templates select="commentAddressItems/commentOriginator"/>
  </xsl:template>

  <xsl:template match="commentPriority">
    <priority>
      <xsl:apply-templates select="@*"/>
    </priority>
  </xsl:template>

  <xsl:template match="@commentPriorityCode">
    <xsl:attribute name="cprio">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="comment/identAndStatusSection">
    <cstatus>
      <xsl:apply-templates select="@*|node()"/>
    </cstatus>
  </xsl:template>

  <xsl:template match="commentStatus">
    <xsl:apply-templates select="security"/>
    <xsl:apply-templates select="commentPriority"/>
    <xsl:apply-templates select="commentResponse"/>
    <xsl:apply-templates select="commentRefs"/>
  </xsl:template>

  <xsl:template match="commentOriginator">
    <corig>
      <xsl:apply-templates select="@*|node()"/>
    </corig>
  </xsl:template>

  <xsl:template match="dispatchAddress">
    <dispaddr>
      <xsl:apply-templates select="@*|node()"/>
    </dispaddr>
  </xsl:template>

  <xsl:template match="enterprise/enterpriseName">
    <ent-name>
      <xsl:apply-templates select="@*|node()"/>
    </ent-name>
  </xsl:template>

  <xsl:template match="commentResponse">
    <response>
      <xsl:apply-templates select="@*"/>
    </response>
  </xsl:template>

  <xsl:template match="@responseType">
    <xsl:attribute name="rsptype">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="commentRefs">
    <crefs>
      <xsl:apply-templates select="@*|node()"/>
    </crefs>
  </xsl:template>

  <xsl:template match="noReferences">
    <cnorefs/>
  </xsl:template>

  <xsl:template match="commentContent">
    <ccontent>
      <xsl:apply-templates select="@*|node()"/>
    </ccontent>
  </xsl:template>

  <xsl:template match="ddnCode">
    <ddnc>
      <xsl:apply-templates select="@*"/>
    </ddnc>
  </xsl:template>

  <xsl:template match="@receiverIdent">
    <recvid>
      <xsl:apply-templates/>
    </recvid>
  </xsl:template>

  <xsl:template match="ddn/identAndStatusSection">
    <xsl:apply-templates select="ddnAddress/ddnIdent/ddnCode"/>
    <xsl:apply-templates select="ddnAddress/ddnAddressItems/issueDate"/>
    <xsl:apply-templates select="ddnStatus/security"/>
    <xsl:apply-templates select="ddnAddress/ddnAddressItems/dispatchTo"/>
    <xsl:apply-templates select="ddnAddress/ddnAddressItems/dispatchFrom"/>
    <xsl:apply-templates select="ddnStatus/authorization"/>
  </xsl:template>

  <xsl:template match="dispatchTo">
    <dispto>
      <xsl:apply-templates/>
    </dispto>
  </xsl:template>

  <xsl:template match="dispatchFrom">
    <dispfrom>
      <xsl:apply-templates/>
    </dispfrom>
  </xsl:template>

  <xsl:template match="authorization">
    <authrtn>
      <xsl:apply-templates/>
    </authrtn>
  </xsl:template>

  <xsl:template match="ddnContent">
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="deliveryList">
    <delivlst>
      <xsl:apply-templates/>
    </delivlst>
  </xsl:template>

</xsl:stylesheet>
