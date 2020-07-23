<?xml version="1.0" encoding="UTF-8"?>
<xsl:transform
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:s1kd-repcheck="urn:s1kd-tools:s1kd-repcheck"
  version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="accessPointRef[@accessPointNumber]|accpnl[@accpnlnbr]">
    <xsl:variable name="apn" select="@accessPointNumber|@accpnlnbr"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="s1kd-repcheck:type">acp</xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:name">
        <xsl:text>Access Point </xsl:text>
        <xsl:value-of select="$apn"/>
      </xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:test">
        <xsl:text>//accessPointIdent[@accessPointNumber='</xsl:text>
        <xsl:value-of select="$apn"/>
        <xsl:text>']|//accpnlid[@accpnlnbr='</xsl:text>
        <xsl:value-of select="$apn"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="applicRef">
    <xsl:variable name="aiv" select="@applicIdentValue"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="s1kd-repcheck:type">app</xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:name">
        <xsl:text>Applic </xsl:text>
        <xsl:value-of select="$aiv"/>
      </xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:test">
        <xsl:text>//applicSpecIdent[@applicIdentValue='</xsl:text>
        <xsl:value-of select="$aiv"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="cautionRef">
    <xsl:variable name="cin" select="@cautionIdentNumber"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="s1kd-repcheck:type">caut</xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:name">
        <xsl:text>Caution </xsl:text>
        <xsl:value-of select="$cin"/>
      </xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:test">
        <xsl:text>//cautionIdent[@cautionIdentNumber='</xsl:text>
        <xsl:value-of select="$cin"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="circuitBreakerRef|cb">
    <xsl:variable name="cbn" select="@circuitBreakerNumber|@cbnbr"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="s1kd-repcheck:type">cbr</xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:name">
        <xsl:text>Circuit breaker </xsl:text>
        <xsl:value-of select="$cbn"/>
      </xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:test">
        <xsl:text>//circuitBreakerIdent[@circuitBreakerNumber='</xsl:text>
        <xsl:value-of select="$cbn"/>
        <xsl:text>']|//cbid[@cbnbr='</xsl:text>
        <xsl:value-of select="$cbn"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="controlIndicatorRef">
    <xsl:variable name="cin" select="@controlIndicatorNumber"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="s1kd-repcheck:type">cin</xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:name">
        <xsl:text>Control/Indicator </xsl:text>
        <xsl:value-of select="$cin"/>
      </xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:test">
        <xsl:text>//controlIndicatorSpec[@controlIndicatorNumber='</xsl:text>
        <xsl:value-of select="$cin"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="responsiblePartnerCompany[@enterpriseCode]|originator[@enterpriseCode]|rpc[text()]|orig[text()]">
    <xsl:variable name="ent">
      <xsl:choose>
        <xsl:when test="self::responsiblePartnerCompany|self::originator">
          <xsl:value-of select="@enterpriseCode"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="."/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="s1kd-repcheck:type">ent</xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:name">
        <xsl:text>Enterprise </xsl:text>
        <xsl:value-of select="$ent"/>
      </xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:test">
        <xsl:text>//enterpriseIdent[@manufacturerCodeValue='</xsl:text>
        <xsl:value-of select="$ent"/>
        <xsl:text>']|//organizationid[@mfc='</xsl:text>
        <xsl:value-of select="$ent"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="functionalItemRef|ein">
    <xsl:variable name="fin" select="@functionalItemNumber|@einnbr"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="s1kd-repcheck:type">fin</xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:name">
        <xsl:text>Functional item </xsl:text>
        <xsl:value-of select="$fin"/>
      </xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:test">
        <xsl:text>//functionalItemIdent[@functionalItemNumber='</xsl:text>
        <xsl:value-of select="$fin"/>
        <xsl:text>']|//einid[@einnbr='</xsl:text>
        <xsl:value-of select="$fin"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="partRef">
    <xsl:variable name="mcv" select="@manufacturerCodeValue"/>
    <xsl:variable name="pnv" select="@partNumberValue"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="s1kd-repcheck:type">part</xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:name">
        <xsl:text>Part </xsl:text>
        <xsl:value-of select="$mcv"/>
        <xsl:text>/</xsl:text>
        <xsl:value-of select="$pnv"/>
      </xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:test">
        <xsl:text>//partIdent[@manufacturerCodeValue='</xsl:text>
        <xsl:value-of select="$mcv"/>
        <xsl:text>' and @partNumberValue='</xsl:text>
        <xsl:value-of select="$pnv"/>
        <xsl:text>']|//partid[@mfc='</xsl:text>
        <xsl:value-of select="$mcv"/>
        <xsl:text>' and @pnr='</xsl:text>
        <xsl:value-of select="pnv"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="supplyRef|con">
    <xsl:variable name="sn" select="@supplyNumber|@connbr"/>
    <xsl:variable name="snt" select="@supplyNumberType"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="s1kd-repcheck:type">supply</xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:name">
        <xsl:text>Supply </xsl:text>
        <xsl:value-of select="$sn"/>
        <xsl:if test="$snt">
          <xsl:text> (</xsl:text>
          <xsl:value-of select="$snt"/>
          <xsl:text>)</xsl:text>
        </xsl:if>
      </xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:test">
        <xsl:text>//supplyIdent[@supplyNumber='</xsl:text>
        <xsl:value-of select="$sn"/>
        <xsl:if test="$snt">
          <xsl:text>' and @supplyNumberType='</xsl:text>
          <xsl:value-of select="$snt"/>
        </xsl:if>
        <xsl:text>']|//conitemid[@itemnbr='</xsl:text>
        <xsl:value-of select="$sn"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="toolRef|tool[@toolnbr]">
    <xsl:variable name="mcv" select="@manufacturerCodeValue|@mfc"/>
    <xsl:variable name="tn" select="@toolNumber|@toolnbr"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="s1kd-repcheck:type">tool</xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:name">
        <xsl:text>Tool </xsl:text>
        <xsl:if test="$mcv">
          <xsl:value-of select="$mcv"/>
          <xsl:text>/</xsl:text>
        </xsl:if>
        <xsl:value-of select="$tn"/>
      </xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:test">
        <xsl:text>//toolIdent[</xsl:text>
        <xsl:if test="$mcv">
          <xsl:text>@manufacturerCodeValue='</xsl:text>
          <xsl:value-of select="$mcv"/>
          <xsl:text>' and </xsl:text>
        </xsl:if>
        <xsl:text>@toolNumber='</xsl:text>
        <xsl:value-of select="$tn"/>
        <xsl:text>']|//toolid[</xsl:text>
        <xsl:if test="$mcv">
          <xsl:text>@mfc='</xsl:text>
          <xsl:value-of select="$mcv"/>
          <xsl:text>' and</xsl:text>
        </xsl:if>
        <xsl:text>@toolnbr='</xsl:text>
        <xsl:value-of select="$tn"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="warningRef">
    <xsl:variable name="win" select="@warningIdentNumber"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="s1kd-repcheck:type">warn</xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:name">
        <xsl:text>Warning </xsl:text>
        <xsl:value-of select="$win"/>
      </xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:test">
        <xsl:text>//warningIdent[@warningIdentNumber='</xsl:text>
        <xsl:value-of select="$win"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="zoneRef[@zoneNumber]|zone[@zonenbr]">
    <xsl:variable name="zn" select="@zoneNumber|@zonenbr"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="s1kd-repcheck:type">zone</xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:name">
        <xsl:text>Zone </xsl:text>
        <xsl:value-of select="$zn"/>
      </xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:test">
        <xsl:text>//zoneIdent[@zoneNumber='</xsl:text>
        <xsl:value-of select="$zn"/>
        <xsl:text>']|//zoneid[@zonenbr='</xsl:text>
        <xsl:value-of select="$zn"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="identNumber[partAndSerialNumber]|identno[pnr]">
    <xsl:variable name="mfc" select="manufacturerCode|mfc"/>
    <xsl:variable name="pnr" select="partAndSerialNumber/partNumber|pnr"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="s1kd-repcheck:type">identno</xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:name">
        <xsl:text>Ident No. </xsl:text>
        <xsl:value-of select="$mfc"/>
        <xsl:text>/</xsl:text>
        <xsl:value-of select="$pnr"/>
      </xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:test">
        <xsl:text>//partIdent[@manufacturerCodeValue='</xsl:text>
        <xsl:value-of select="$mfc"/>
        <xsl:text>' and @partNumberValue='</xsl:text>
        <xsl:value-of select="$pnr"/>
        <xsl:text>']|//partid[@mfc='</xsl:text>
        <xsl:value-of select="$mfc"/>
        <xsl:text>' and @pnr='</xsl:text>
        <xsl:value-of select="$pnr"/>
        <xsl:text>']|//supplyIdent[@supplyNumber='</xsl:text>
        <xsl:value-of select="$pnr"/>
        <xsl:text>']|//conitemid[@itemnbr='</xsl:text>
        <xsl:value-of select="$pnr"/>
        <xsl:text>']|//toolIdent[</xsl:text>
        <xsl:text>@manufacturerCodeValue='</xsl:text>
        <xsl:value-of select="$mfc"/>
        <xsl:text>' and </xsl:text>
        <xsl:text>@toolNumber='</xsl:text>
        <xsl:value-of select="$pnr"/>
        <xsl:text>']|//toolid[</xsl:text>
        <xsl:text>@mfc='</xsl:text>
        <xsl:value-of select="$mfc"/>
        <xsl:text>' and</xsl:text>
        <xsl:text>@toolnbr='</xsl:text>
        <xsl:value-of select="$pnr"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="spareDescr/identNumber[partAndSerialNumber]|spare/identno[pnr]">
    <xsl:variable name="mcv" select="manufacturerCode|mfc"/>
    <xsl:variable name="pnr" select="partAndSerialNumber/partNumber|pnr"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="s1kd-repcheck:type">part</xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:name">
        <xsl:text>Part </xsl:text>
        <xsl:value-of select="$mcv"/>
        <xsl:text>/</xsl:text>
        <xsl:value-of select="$pnr"/>
      </xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:test">
        <xsl:text>//partIdent[@manufacturerCodeValue='</xsl:text>
        <xsl:value-of select="$mcv"/>
        <xsl:text>' and @partNumberValue='</xsl:text>
        <xsl:value-of select="$pnr"/>
        <xsl:text>']|//partid[@mfc='</xsl:text>
        <xsl:value-of select="$mcv"/>
        <xsl:text>' and @pnr='</xsl:text>
        <xsl:value-of select="$pnr"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="supplyDescr/identNumber[partAndSerialNumber]|supply/identno[pnr]">
    <xsl:variable name="sn" select="partAndSerialNumber/partNumber|pnr"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="s1kd-repcheck:type">supply</xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:name">
        <xsl:text>Supply </xsl:text>
        <xsl:value-of select="$sn"/>
      </xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:test">
        <xsl:text>//supplyIdent[@supplyNumber='</xsl:text>
        <xsl:value-of select="$sn"/>
        <xsl:text>']|//conitemid[@itemnbr='</xsl:text>
        <xsl:value-of select="$sn"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="supportEquipDescr/identNumber[partAndSerialNumber]|supequi/identno[pnr]">
    <xsl:variable name="mcv" select="manufacturerCode|mfc"/>
    <xsl:variable name="tn" select="partAndSerialNumber/partNumber|pnr"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="s1kd-repcheck:type">tool</xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:name">
        <xsl:text>Tool </xsl:text>
        <xsl:value-of select="$mcv"/>
        <xsl:text>/</xsl:text>
        <xsl:value-of select="$tn"/>
      </xsl:attribute>
      <xsl:attribute name="s1kd-repcheck:test">
        <xsl:text>//toolIdent[</xsl:text>
        <xsl:text>@manufacturerCodeValue='</xsl:text>
        <xsl:value-of select="$mcv"/>
        <xsl:text>' and </xsl:text>
        <xsl:text>@toolNumber='</xsl:text>
        <xsl:value-of select="$tn"/>
        <xsl:text>']|//toolid[</xsl:text>
        <xsl:text>@mfc='</xsl:text>
        <xsl:value-of select="$mcv"/>
        <xsl:text>' and</xsl:text>
        <xsl:text>@toolnbr='</xsl:text>
        <xsl:value-of select="$tn"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

</xsl:transform>
