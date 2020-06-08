<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">

  <xsl:key name="icn" match="figure/graphic" use="@infoEntityIdent"/>

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="/">
    <xsl:variable name="graphics" select="//figure/graphic[generate-id() = generate-id(key('icn', @infoEntityIdent))]"/>
    <content>
      <description>
        <table pgwide="1">
          <tgroup cols="2">
            <colspec colname="c1"/>
            <colspec colname="c2"/>
            <thead>
              <row>
                <entry>
                  <para>ICN</para>
                </entry>
                <entry>
                  <para>Title</para>
                </entry>
              </row>
            </thead>
            <tbody>
              <xsl:choose>
                <xsl:when test="$graphics">
                  <xsl:apply-templates select="$graphics"/>
                </xsl:when>
                <xsl:otherwise>
                  <row>
                    <entry namest="c1" nameend="c2">
                      <para>None</para>
                    </entry>
                  </row>
                </xsl:otherwise>
              </xsl:choose>
            </tbody>
          </tgroup>
        </table>
      </description>
    </content>
  </xsl:template>

  <xsl:template match="graphic">
    <row>
      <entry>
        <para>
          <xsl:value-of select="@infoEntityIdent"/>
        </para>
      </entry>
      <entry>
        <para>
          <xsl:value-of select="parent::figure/title"/>
        </para>
      </entry>
    </row>
  </xsl:template>

</xsl:stylesheet>
