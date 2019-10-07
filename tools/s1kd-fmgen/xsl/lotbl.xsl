<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="/">
    <xsl:variable name="tables" select="//table[title]"/>
    <content>
      <description>
        <table pgwide="1">
          <tgroup cols="2">
            <colspec colname="c1"/>
            <colspec colname="c2"/>
            <thead>
              <row>
                <entry>
                  <para>Data module</para>
                </entry>
                <entry>
                  <para>Title</para>
                </entry>
              </row>
            </thead>
            <tbody>
              <xsl:choose>
                <xsl:when test="$tables">
                  <xsl:apply-templates select="$tables"/>
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

  <xsl:template match="table">
    <xsl:variable name="dmodule" select="ancestor::dmodule"/>
    <row>
      <entry>
        <para>
          <dmRef>
            <dmRefIdent>
              <xsl:apply-templates select="$dmodule/identAndStatusSection/dmAddress/dmIdent/dmCode"/>
              <xsl:apply-templates select="$dmodule/identAndStatusSection/dmAddress/dmIdent/issueInfo"/>
              <xsl:apply-templates select="$dmodule/identAndStatusSection/dmAddress/dmIdent/language"/>
            </dmRefIdent>
            <dmRefAddressItems>
              <xsl:apply-templates select="$dmodule/identAndStatusSection/dmAddress/dmAddressItems/dmTitle"/>
              <xsl:apply-templates select="$dmodule/identAndStatusSection/dmAddress/dmAddressItems/issueDate"/>
            </dmRefAddressItems>
          </dmRef>
        </para>
      </entry>
      <entry>
        <para>
          <xsl:value-of select="title"/>
        </para>
      </entry>
    </row>
  </xsl:template>

</xsl:stylesheet>
