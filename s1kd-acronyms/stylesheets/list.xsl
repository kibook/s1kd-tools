<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="acronyms">
    <definitionList>
      <xsl:apply-templates select="acronym"/>
    </definitionList>
  </xsl:template>

  <xsl:template match="acronym">
    <definitionListItem>
      <xsl:apply-templates select="acronymTerm|acronymDefinition"/>
    </definitionListItem>
  </xsl:template>

  <xsl:template match="acronymTerm">
    <listItemTerm>
      <xsl:apply-templates/>
    </listItemTerm>
  </xsl:template>

  <xsl:template match="acronymDefinition">
    <listItemDefinition>
      <para>
        <xsl:apply-templates/>
      </para>
    </listItemDefinition>
  </xsl:template>

</xsl:stylesheet>
