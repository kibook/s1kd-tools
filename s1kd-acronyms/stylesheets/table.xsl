<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  
  <xsl:template match="acronyms">
    <table>
      <tgroup cols="2">
        <tbody>
          <xsl:apply-templates select="acronym"/>
        </tbody>
      </tgroup>
    </table>
  </xsl:template>

  <xsl:template match="acronym">
    <row>
      <xsl:apply-templates select="acronymTerm|acronymDefinition"/>
    </row>
  </xsl:template>

  <xsl:template match="acronymTerm|acronymDefinition">
    <entry>
      <para>
        <xsl:apply-templates/>
      </para>
    </entry>
  </xsl:template>

</xsl:stylesheet>
