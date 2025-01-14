#include <stdint.h>
#include <string.h>

#include <libexslt/exslt.h>
#include <libxml/parser.h>
#include <libxml/xmlversion.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

#include "xslt.h"

/*
 * Function: apply_style
 * ----------------------------
 * Restyle an XML document using a parsed XSL stylesheet.
 *
 *   style:        parsed XSL stylesheet
 *   xml:          XML to be transformed
 *   params:       parameters to be passed
 *   xml_txt:      output from the transform
 *   xml_txt_len:  length in bytes of output
 *
 *  returns 0 if the transform is successful or -1 in case of error
 */
int apply_style(xsltStylesheetPtr style, const char *xml, const char **params,
                char **xml_txt, size_t *xml_txt_len) {

  int ok;
  size_t len;
  xmlChar *xml_output;
  xmlDocPtr xml_doc, result;

  // avoid overflow on conversion from size_t to int
  len = strlen(xml);
  if (len > INT32_MAX) {
    return -1;
  }

  // parse the provided xml document
  xml_doc = xmlParseMemory(xml, (int)strlen(xml));
  if (xml_doc == NULL || xmlGetLastError()) {
    xmlResetLastError();
    return -1;
  }

  // obtain the result from transforming xml_doc using the style
  result = xsltApplyStylesheet(style, xml_doc, params);
  if (result == NULL) {
    xmlFreeDoc(xml_doc);
    return -1;
  }

  // save the transformation result
  ok = xsltSaveResultToString(&xml_output, (int *)xml_txt_len, result, style);
  if (ok == 0 && *xml_txt_len > 0) {
    *xml_txt = malloc(*xml_txt_len);
    strncpy(*xml_txt, (const char *)xml_output, *xml_txt_len);
    xmlFree(xml_output);
  }

  xmlFreeDoc(xml_doc);
  xmlFreeDoc(result);

  return ok;
}

/*
 * Function: free_style
 * ----------------------------
 * Free memory allocated by the style.
 * Note that the stylesheet document (style_doc) is also automatically
 * freed. See: http://xmlsoft.org/XSLT/html/libxslt-xsltInternals.html
 *  @ #xsltParseStylesheetDoc.
 *
 *   style:        an XSL stylesheet pointer
 *
 *  returns void
 */
void free_style(xsltStylesheetPtr *style) { xsltFreeStylesheet(*style); }

/*
 * Function: make_style
 * ----------------------------
 * Parse an XSL stylesheet.
 *
 *   xsl:          XSL to be transformed
 *   style:        parse XSL stylesheet
 *
 *  returns 0 if parsing is successful or -1 in case of error
 */
int make_style(const char *xsl, xsltStylesheetPtr *style) {

  size_t len;
  xmlDocPtr style_doc;

  len = strlen(xsl);
  if (len > INT32_MAX) {
    return -1;
  }

  style_doc = xmlParseMemory(xsl, (int)len);
  if (style_doc == NULL || xmlGetLastError()) {
    xmlResetLastError();
    return -1;
  }

  *style = xsltParseStylesheetDoc(style_doc);
  if (*style == NULL || (*style)->errors) {
    xmlFreeDoc(style_doc);
    return -1;
  }

  return 0;
}

/*
 * Function: xslt
 * ----------------------------
 *  Transforms an XML document using an XSL stylesheet.
 *
 *   xsl:          the stylesheet to be used
 *   xml:          the document to transform
 *   params:       parameters to be passed
 *   xml_txt:      output from the transform
 *   xml_txt_len:  length in bytes of output
 *
 *  returns 0 if the transform is successful or -1 in case of error
 */
int xslt(const char *xsl, const char *xml, const char **params, char **xml_txt,
         size_t *xml_txt_len) {

  int ok;
  xsltStylesheetPtr style;

  ok = make_style(xsl, &style);
  if (ok < 0) {
    return -1;
  }

  ok = apply_style(style, xml, params, xml_txt, xml_txt_len);

  free_style(&style);

  return ok;
}

/*
 * Function: init_exslt
 * ----------------------------
 *  Calls exsltRegisterAll() to enable exsl namespace at templates
 */
void init_exslt() {
  LIBXML_TEST_VERSION

  xmlInitParser();
  xsltInit();
  exsltRegisterAll();
}

const char **make_param_array(int num_tuples) {
  const char **a = calloc(sizeof(char *), 2 * num_tuples + 1);
  a[2 * num_tuples] = NULL;
  return a;
}

void set_param(char **a, char *n, char *v, int t) {
  a[2 * t] = n;
  a[2 * t + 1] = v;
}

void free_param_array(char **a, int num_tuples) {
  int i;
  for (i = 0; i < 2 * num_tuples; i++)
    free(a[i]);
  free(a);
}
