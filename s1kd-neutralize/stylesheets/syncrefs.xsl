<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  
  <xsl:template match="node()|@*">
    <xsl:copy>
      <xsl:apply-templates select="node()|@*"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="content">
    <xsl:copy>
      <xsl:choose>
        <xsl:when test="refs">
          <xsl:apply-templates select="refs"/>
          <xsl:apply-templates select="node()[not(self::refs)]|@*"/>
        </xsl:when>
        <xsl:otherwise>
          <refs>
            <xsl:call-template name="add.refs"/>
          </refs>
          <xsl:apply-templates select="node()|@*"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:copy>
  </xsl:template>
  
  <xsl:template match="refs">
    <xsl:copy>
      <xsl:call-template name="add.refs"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template name="add.refs">
    <xsl:for-each select="//description//dmRef|//procedure//dmRef">
      <xsl:sort/>
      <xsl:copy>
        <xsl:apply-templates select="node()|@*"/>
      </xsl:copy>
    </xsl:for-each>
    <xsl:for-each select="//description//pmRef|//procedure//dmRef">
      <xsl:sort/>
      <xsl:copy>
        <xsl:apply-templates select="node()|@*"/>
      </xsl:copy>
    </xsl:for-each>
    <xsl:for-each select="//description//externalPubRef|//procedure//externalPubRef">
      <xsl:sort/>
      <xsl:copy>
        <xsl:apply-templates select="node()|@*"/>
      </xsl:copy>
    </xsl:for-each>
  </xsl:template>
  
</xsl:stylesheet>
