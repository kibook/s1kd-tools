<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="functionalItemRef">
    <xsl:variable name="fin" select="@functionalItemNumber"/>
    <xsl:variable name="fit" select="@functionalItemType"/>
    <xsl:variable name="install" select="@installationIdent"/>
    <xsl:variable name="context" select="@contextIdent"/>
    <xsl:variable name="mcv" select="@manufacturerCodeValue"/>
    <xsl:variable name="orig" select="@itemOriginator"/>
    
    <xsl:variable name="ident" select="
      //functionalItemIdent[
        $fin = @functionalItemNumber and
        (not($fit) or $fit = @functionalItemType) and
        (not($install) or $install = @installationIdent) and
        (not($context) or $context = @contextIdent) and
        (not($mcv) or $mcv = @manufacturerCodeValue) and
        (not($orig) or $orig = @itemOriginator)
      ]"/>

    <functionalItemRef>
      <xsl:choose>
        <xsl:when test="$ident">
          <xsl:variable name="spec" select="$ident/parent::functionalItemSpec"/>
          <xsl:copy-of select="@id"/>
          <xsl:copy-of select="$ident/@functionalItemNumber"/>
          <xsl:copy-of select="$ident/@functionalItemType"/>
          <xsl:copy-of select="$ident/@installationIdent"/>
          <xsl:copy-of select="$ident/@contextIdent"/>
          <xsl:copy-of select="$ident/@manufacturerCodeValue"/>
          <xsl:copy-of select="$ident/@itemOriginator"/>
          <xsl:copy-of select="$spec/name"/>
          <xsl:copy-of select="$spec/shortName"/>
          <xsl:copy-of select="$spec/refs"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="@*|node()"/>
        </xsl:otherwise>
      </xsl:choose>
    </functionalItemRef>
  </xsl:template>

</xsl:stylesheet>
