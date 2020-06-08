<?xml version="1.0"?>
<p:pipeline
  xmlns:p="http://www.w3.org/ns/xproc"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">
  <p:xslt name="Pass 1">
    <p:input port="stylesheet">
      <p:document href="pass1.xsl"/>
    </p:input>
  </p:xslt>
  <p:xslt name="Pass 2">
    <p:input port="stylesheet">
      <p:document href="pass2.xsl"/>
    </p:input>
    <p:with-param name="title" select="'Alternative title'"/>
  </p:xslt>
  <p:xslt name="Pass 3">
    <p:input port="stylesheet">
      <p:inline>
        <xsl:stylesheet version="1.0">
          <xsl:template match="@*|node()">
            <xsl:copy>
              <xsl:apply-templates select="@*|node()"/>
            </xsl:copy>
          </xsl:template>
          <xsl:template match="frontMatterTitlePage">
            <xsl:comment>This was inserted by the third pass.</xsl:comment>
            <xsl:copy>
              <xsl:apply-templates select="@*|node()"/>
            </xsl:copy>
          </xsl:template>
        </xsl:stylesheet>
      </p:inline>
    </p:input>
  </p:xslt>
</p:pipeline>
