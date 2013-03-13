<?php

/**
 * Performs a Solr query, adding in some defaults we need for highlighting.
 *
 * @param string $query
 *   A Solr query string.
 * @param array $params
 *   An array of Solr parameters. Probably want to throw in at least an fq, to
 *   restrict the results to those you are interested in.
 *
 * @return array
 *   An array as per islandora_paged_content_parse_solr_highlighting().
 */
function islandora_paged_content_perform_solr_highlighting_query($query, $params = array()) {
  $params = array(
    'fl' => array(
      'PID',
      variable_get('islandora_paged_content_sequence_number_field', 'RELS_EXT_isSequenceNumber_literal_ms'),
    ),
    'hl' => 'true',
    'qt' => variable_get('islandora_solr_request_handler', 'standard'),
    'hl.fl' => variable_get('islandora_paged_content_hocr_highlighting_field', 'text_nodes_HOCR_hlt'),
    'hl.useFastVectorHighlighter' => 'true',
    // XXX: Could end up with a large fragment size with fielded
    // searches... Don't really see it as a major issue, though.
    // XXX: We add a bit to the fragsize, so we should get any characters
    // surrounding the fragment--on either side--to be able to account for the
    // fragment Solr selects vs what the first token may start with or the last
    // may end with in the HOCR.
    "hl.fragsize" => (int)(drupal_strlen($query) * 1.10),
    "hl.snippets" => "8",
  ) + $params;

  $params['fl'] = implode(',', $params['fl']);

  $path_parts = parse_url(variable_get('islandora_solr_url', 'localhost:8080/solr'));
  $solr = new Apache_Solr_Service($path_parts['host'], $path_parts['port'], $path_parts['path'] . '/');
  $solr->setCreateDocuments(0);

  // Query is executed.
  try {
    $results = $solr->search($query, 0, variable_get('islandora_paged_content_solr_result_count', '32'), $params);
    $results = json_decode($results->getRawResponse(), TRUE);
    return islandora_paged_content_parse_solr_highlighting($results);
  }
  catch (Exception $e) {
    watchdog_exception('islandora_paged_content', $e, 'Got an exception while searching.', array(), WATCHDOG_ERROR);
  }
}

/**
 * Parse the Solr highlighting section in a manner we can reuse.
 *
 * @param array $solr_results
 *   The Solr results; deserialized JSON.
 *
 * @return array
 *   An associative array mapping PIDs to snippets to an array containing
 *   the parameters for the bounding box of the given object.
 */
function islandora_paged_content_parse_solr_highlighting($solr_results) {
  $results = array();

  $params = $solr_results['responseHeader']['params'];
  if (isset($params['hl.useFastVectorHighlighter']) && $params['hl.useFastVectorHighlighter'] == 'true') {
    $hl_pre = isset($params['hl.tag.pre']) ? $params['hl.tag.pre'] : '<em>';
    $hl_post = isset($params['hl.tag.post']) ? $params['hl.tag.post'] : '</em>';
  }
  else {
    $hl_pre = isset($params['hl.simple.pre']) ? $params['hl.simple.pre'] : '<em>';
    $hl_post = isset($params['hl.simple.post']) ? $params['hl.simple.post'] : '</em>';
  }

  $d = '/';
  $not_whitespace = '([^\s]*)';
  $snippet_content_pattern = $d . '(' . $not_whitespace .
    preg_quote($hl_pre, $d) . '(.+)' . preg_quote($hl_post, $d) .
    $not_whitespace . ')' . $d;
  $temp_name = drupal_realpath(drupal_tempnam('temporary://', 'hocrfile'));
  $dom = new DOMDocument();

  $page_field_name = variable_get('islandora_paged_content_sequence_number_field', 'RELS_EXT_isSequenceNumber_literal_ms');
  foreach ($solr_results['response']['docs'] as $doc) {
    $id = $doc['PID'];
    $fields = $solr_results['highlighting'][$id];

    $results[$id] = array(
      'page' => array(),
      'snippets' => array(),
    );

    if (isset($doc[$page_field_name])) {
      $pages = (array)$doc[$page_field_name];
      $results[$id]['page']['sequence_number'] = reset($pages);
    }

    $obj = islandora_object_load($id);
    if (isset($obj['HOCR'])) {
      $obj['HOCR']->getContent($temp_name);
      $hocr = new SimpleXMLElement($temp_name, 0, TRUE);
      $hocr->registerXPathNamespace('x', 'http://www.w3.org/1999/xhtml');
      $words = $hocr->xpath('//x:span[@class="ocrx_word" and @title]');

      // Put the width and height in...
      // XXX: Would end up taking only the last page, if there were multiple.
      $page_matches = $hocr->xpath("//x:div[@class='ocr_page' and @title]");
      if ($page_matches) {
        foreach ($page_matches as $match) {
          $page_dimensions = (string)$match['title'];
          $values = explode(' ', $page_dimensions);
          $results[$id]['page']['width'] = intval($values[5]);
          $results[$id]['page']['height'] = intval($values[6]);
        }
      }
      else {
        // We need dimensions from somewhere...
        module_load_include('inc', 'islandora_paged_content', 'includes/utilities');
        $results[$id]['page'] += islandora_paged_content_get_page_dimensions($obj);
      }

      foreach ($fields as $field => $snippets) {
        foreach ($snippets as $snippet) {
          // Get matches of tokens contained between the highlighting indicators.
          $matches = array();
          if(preg_match_all($snippet_content_pattern, $snippet, $matches)) {
            // For each match, finds all occurences of the given tokens in the
            // given order...
            foreach ($matches[1] as $index => $match) {
              // Split up into separate tokens.
              // XXX: Because Solr can ignore some characters included inside
              // of the hOCR at the start or end of selected fragment, in is
              // necessary to grab non-whitespace characters to either side of
              // the fragment returned, so we can match against the actual
              // HOCR.
              $tokens = preg_split('/\s+/', format_string('!before_pre!solr_selected_fragment!after_post', array(
                '!before_pre' => $matches[2][$index],
                '!solr_selected_fragment' => $matches[3][$index],
                '!after_post' => $matches[4][$index],
              )));

              // For each match, produce a structure containing the bounding
              //   boxes.
              $spans = islandora_paged_content_get_phrase_from_hocr($words, $tokens);
              foreach ($spans as $span) {
                $box = islandora_paged_content_get_bounding_box((string)$span['title']);
                $results[$id]['snippets'][$snippet][] = $box;
              }
            }
          }
        }
      }
    }
  }

  drupal_unlink($temp_name);

  return $results;
}

/**
 * Get the <span>s corresponding the the series of phrase.
 *
 * XPath can't really do sequences... Instead, we have all the spans holding
 * words given to us.
 *
 * @param array $words
 *   An array of SimpleXMLElements representing the <span>s.
 * @param array $phrase
 *   AN array of strings, representing a phrase to look for in the words.
 */
function islandora_paged_content_get_phrase_from_hocr($words, $phrase) {
  $spans = array();

  // Find where the phrase might start.
  $relevant_indicies = array_keys($words, $phrase[0]);

  // Get the number of terms in the phrase.
  $number_of_phrase_terms = count($phrase);

  // For each index where the phrase might start...
  foreach($relevant_indicies as $index) {
    $match = TRUE;

    // Try to match all the terms in the phrase against those following the our
    // previously indicated index.
    for ($i = 1; $i < $number_of_phrase_terms && $match; $i++) {
      $match = ((string)$words[$index + $i]) == $phrase[$i];
    }

    // If we've a complete match, get the relevant elements to return.
    if ($match) {
      $spans = array_merge($spans, array_slice($words, $index, $number_of_phrase_terms));
    }
  }

  return $spans;
}

/**
 * Get the word coordinates for the given term and page.
 *
 * @param string $pid
 *   The PID of the page object to check.
 * @param string $page
 *   The page index.
 * @param string $terms
 *   The terms to fetch the coordinates for. Terms Seperated by a single space.
 *
 * @return array
 *   The word coordinates for the given term and page.
 */
function islandora_paged_content_exact_term_match($object, $hocr_sxml, $terms) {
  // Setup our DOMXPath...
  $dom = dom_import_simplexml($hocr_sxml);
  $doc = new DOMDocument('1.0', 'UTF-8');
  $doc->appendChild($dom);
  $xpath = new DOMXPath($doc);
  $xpath->registerNamespace("php", "http://php.net/xpath");
  $xpath->registerPhpFunctions();

  // Transform the potentially Lucene-like output to something we should be
  // able to use
  $terms = $info['solr']['query'];
  // First remove any NOT that may be present.
  $terms = preg_replace('/-\((.*)\)/', '', $query_string);
  // Next remove any ANDs, ORs, (s or )s.
  $terms = str_replace(array(' &&', ' ||', '"', ')', '('), '', $terms);
  $terms = mb_strtolower($terms);
  $terms = urldecode($terms);
  $terms = explode(' ', $terms);
  $terms = array_map('trim', $terms);
  $terms = array_filter($terms, function ($term) {
    return strpos($term, '-') !== 0;
  });

  $boxes = array();
  foreach ($terms as $term) {
    $matches = $xpath->query("//*[@title and contains(@class, 'ocrx_word') and php:functionString('mb_strtolower', text()) = '{$term}']");
    foreach ($matches as $match) {
      $bounding_box = islandora_paged_content_get_bounding_box($match->getAttribute('title'));
      if ($bounding_box) {
        $boxes[] = $box;
      }
    }
  }
  return $boxes;
}

/**
 * Gets the bounding box for the given element if defined.
 *
 * @param string $title_attribute
 *   The contents of the title attribute to grab the bounding box from.
 *
 * @return array
 *   An associative array as follows:
 *   - left: The left most point of the element in pixels.
 *   - top: The top most point of the element in pixels.
 *   - right: The right most point of the element in pixels.
 *   - bottom: The bottom most point of the element in pixels.
 *  If the given element doesn't have a bounding box FALSE is returned.
 */
function islandora_paged_content_get_bounding_box($title_attribute) {
  if (!empty($title_attribute)) {
    // The coordinates are stored in the title attribute in the following
    // format "bbox 539 1210 623 1235"
    $bounding_box = $title_attribute;
    $coordinates = explode(' ', $bounding_box);
    return array(
      'left' => intval($coordinates[1]),
      'top' => intval($coordinates[2]),
      'right' => intval($coordinates[3]),
      'bottom' => intval($coordinates[4]),
    );
  }
  return FALSE;
}
