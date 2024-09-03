<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  version="1.0">

  <xsl:variable name="schema-prefix">http://www.s1000d.org/S1000D_6/xml_schema_flat/</xsl:variable>
  
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="@xsi:noNamespaceSchemaLocation">
    <xsl:attribute name="xsi:noNamespaceSchemaLocation">
      <xsl:text>http://www.s1000d.org/S1000D_5-0/xml_schema_flat/</xsl:text>
      <xsl:value-of select="substring-after(., $schema-prefix)"/>
    </xsl:attribute>
  </xsl:template>

</xsl:stylesheet>
