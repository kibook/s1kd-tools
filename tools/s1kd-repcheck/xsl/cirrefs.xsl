<?xml version="1.0" encoding="UTF-8"?>
<xsl:transform
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="accessPointRef[@accessPointNumber]">
    <xsl:variable name="apn" select="@accessPointNumber"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="repcheck_name">
        <xsl:text>Access Point </xsl:text>
        <xsl:value-of select="$apn"/>
      </xsl:attribute>
      <xsl:attribute name="repcheck_test">
        <xsl:text>//accessPointIdent[@accessPointNumber='</xsl:text>
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
      <xsl:attribute name="repcheck_name">
        <xsl:text>Applic </xsl:text>
        <xsl:value-of select="$aiv"/>
      </xsl:attribute>
      <xsl:attribute name="repcheck_test">
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
      <xsl:attribute name="repcheck_name">
        <xsl:text>Caution </xsl:text>
        <xsl:value-of select="$cin"/>
      </xsl:attribute>
      <xsl:attribute name="repcheck_test">
        <xsl:text>//cautionIdent[@cautionIdentNumber='</xsl:text>
        <xsl:value-of select="$cin"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="circuitBreakerRef">
    <xsl:variable name="cbn" select="@circuitBreakerNumber"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="repcheck_name">
        <xsl:text>Circuit breaker </xsl:text>
        <xsl:value-of select="$cbn"/>
      </xsl:attribute>
      <xsl:attribute name="repcheck_test">
        <xsl:text>//circuitBreakerIdent[@circuitBreakerNumber='</xsl:text>
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
      <xsl:attribute name="repcheck_name">
        <xsl:text>Control/Indicator </xsl:text>
        <xsl:value-of select="$cin"/>
      </xsl:attribute>
      <xsl:attribute name="repcheck_test">
        <xsl:text>//controlIndicatorSpec[@controlIndicatorNumber='</xsl:text>
        <xsl:value-of select="$cin"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="functionalItemRef">
    <xsl:variable name="fin" select="@functionalItemNumber"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="repcheck_name">
        <xsl:text>Functional item </xsl:text>
        <xsl:value-of select="$fin"/>
      </xsl:attribute>
      <xsl:attribute name="repcheck_test">
        <xsl:text>//functionalItemIdent[@functionalItemNumber='</xsl:text>
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
      <xsl:attribute name="repcheck_name">
        <xsl:text>Part </xsl:text>
        <xsl:value-of select="$mcv"/>
        <xsl:text>/</xsl:text>
        <xsl:value-of select="$pnv"/>
      </xsl:attribute>
      <xsl:attribute name="repcheck_test">
        <xsl:text>//partIdent[@manufacturerCodeValue='</xsl:text>
        <xsl:value-of select="$mcv"/>
        <xsl:text>' and @partNumberValue='</xsl:text>
        <xsl:value-of select="$pnv"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="supplyRef">
    <xsl:variable name="sn" select="@supplyNumber"/>
    <xsl:variable name="snt" select="@supplyNumberType"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="repcheck_name">
        <xsl:text>Supply </xsl:text>
        <xsl:value-of select="$sn"/>
        <xsl:text> (</xsl:text>
        <xsl:value-of select="$snt"/>
        <xsl:text>)</xsl:text>
      </xsl:attribute>
      <xsl:attribute name="repcheck_test">
        <xsl:text>//supplyIdent[@supplyNumber='</xsl:text>
        <xsl:value-of select="$sn"/>
        <xsl:text>' and @supplyNumberType='</xsl:text>
        <xsl:value-of select="$snt"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="toolRef">
    <xsl:variable name="mcv" select="@manufacturerCodeValue"/>
    <xsl:variable name="tn" select="@toolNumber"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="repcheck_name">
        <xsl:text>Tool </xsl:text>
        <xsl:value-of select="$mcv"/>
        <xsl:text>/</xsl:text>
        <xsl:value-of select="$tn"/>
      </xsl:attribute>
      <xsl:attribute name="repcheck_test">
        <xsl:text>//toolIdent[@manufacturerCodeValue='</xsl:text>
        <xsl:value-of select="$mcv"/>
        <xsl:text>' and @toolNumber='</xsl:text>
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
      <xsl:attribute name="repcheck_name">
        <xsl:text>Warning </xsl:text>
        <xsl:value-of select="$win"/>
      </xsl:attribute>
      <xsl:attribute name="repcheck_test">
        <xsl:text>//warningIdent[@warningIdentNumber='</xsl:text>
        <xsl:value-of select="$win"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="zoneRef[@zoneNumber]">
    <xsl:variable name="zn" select="@zoneNumber"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="repcheck_name">
        <xsl:text>Zone </xsl:text>
        <xsl:value-of select="$zn"/>
      </xsl:attribute>
      <xsl:attribute name="repcheck_test">
        <xsl:text>//zoneIdent[@zoneNumber='</xsl:text>
        <xsl:value-of select="$zn"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="responsiblePartnerCompany[@enterpriseCode]|originator[@enterpriseCode]">
    <xsl:variable name="ent" select="@enterpriseCode"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="repcheck_name">
        <xsl:text>Enterprise </xsl:text>
        <xsl:value-of select="$ent"/>
      </xsl:attribute>
      <xsl:attribute name="repcheck_test">
        <xsl:text>//enterpriseIdent[@manufacturerCodeValue='</xsl:text>
        <xsl:value-of select="$ent"/>
        <xsl:text>']</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

</xsl:transform>
