<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="assocWarningMalfunctionAlts|
                       bitMessageAlts|
                       commonInfoDescrParaAlts|
                       correlatedFaultAlts|
                       detectedFaultAlts|
                       dialogAlts|
                       dmNodeAlts|
                       dmSeqAlts|
                       electricalEquipAlts|
                       figureAlts|
                       harnessAlts|
                       isolatedFaultAlts|
                       isolationProcedureEndAlts|
                       isolationStepAlts|
                       levelledParaAlts|
                       messageAlts|
                       multimediaAlts|
                       observedFaultAlts|
                       proceduralStepAlts|
                       taskDefinitionAlts|
                       warningMalfunctionAlts|
                       wireAlts">
    <xsl:choose>
      <xsl:when test="count(*) = 1">
        <xsl:for-each select="*">
          <xsl:copy>
            <xsl:apply-templates select="parent::*/@id|@*[name() != 'id']|node()"/>
          </xsl:copy>
        </xsl:for-each>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy>
          <xsl:apply-templates select="@*|node()"/>
        </xsl:copy>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>
