<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">

  <xsl:key name="acronym" match="acronym[@acronymType='at03']" use="acronymTerm"/>

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="/">
    <xsl:variable name="acronyms" select="//acronym[generate-id() = generate-id(key('acronym', acronymTerm))]"/>
    <content>
      <description>
        <para>
          <xsl:choose>
            <xsl:when test="$acronyms">
              <definitionList>
                <xsl:apply-templates select="$acronyms">
                  <xsl:sort select="acronymTerm"/>
                </xsl:apply-templates>
              </definitionList>
            </xsl:when>
            <xsl:otherwise>None</xsl:otherwise>
          </xsl:choose>
        </para>
      </description>
    </content>
  </xsl:template>

  <xsl:template match="acronym">
    <definitionListItem>
      <listItemTerm>
        <xsl:value-of select="acronymTerm"/>
      </listItemTerm>
      <listItemDefinition>
        <para>
          <xsl:value-of select="acronymDefinition"/>
        </para>
      </listItemDefinition>
    </definitionListItem>
  </xsl:template>

</xsl:stylesheet>
