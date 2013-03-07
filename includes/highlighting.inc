<?php

function islandora_paged_content_perform_solr_query($query, $params = array()) {
  $params = array_merge_recursive($params, array(
    'fl' => array(
      'PID'
    ),
    'hl' => 'true',
    'qt' => variable_get('islandora_solr_request_handler', 'standard'),
    'hl.fl' => variable_get('islandora_paged_content_hocr_highlighting_field', 'text_nodes_HOCR_hlt'),
    'hl.useFastVectorHighlighter' => 'true',
    "hl.fragsize" => "18",
    "hl.snippets" => "1000",
    "hl.fragmenter" => "gap",
  ));
  $params['fl'] = implode(',', $params['fl']);

  $path_parts = parse_url(variable_get('islandora_solr_url', 'localhost:8080/solr'));
  $solr = new Apache_Solr_Service($path_parts['host'], $path_parts['port'], $path_parts['path'] . '/');
  $solr->setCreateDocuments(0);

  // Query is executed.
  try {
    $results = $solr->search($query, 0, 1, $params);
    $results = json_decode($results->getRawResponse(), TRUE);
    return islandora_paged_content_parse_solr_highlighting($results);
  }
  catch (Exception $e) {
    watchdog_exception('islandora_paged_content', $e, 'Got an exception while searching.', array(), WATCHDOG_ERROR);
  }
}

function islandora_paged_content_parse_solr_highlighting($solr_results) {
  $results = array();

  $hl_pre = isset($solr_results['hl.tag.pre']) ? $solr_result['hl.tag.pre'] : '<em>';
  $hl_post = isset($solr_results['hl.tag.post']) ? $solr_result['hl.tag.post'] : '</em>';
  $d = '/';
  $snippet_content_pattern = $d . preg_quote($hl_pre, $d) . '([\w\s]+)' . preg_quote($hl_post, $d) . $d;

  $temp_name = drupal_realpath(drupal_tempnam('temporary://', 'hocrfile'));
  $dom = new DOMDocument();

  foreach ($solr_results['highlighting'] as $id => $fields) {
    $results[$id] = array();

    $obj = islandora_object_load($id);
    if (isset($obj['HOCR'])) {
      $obj['HOCR']->getContent($temp_name);
      $hocr = new SimpleXMLElement($temp_name, 0, TRUE);
      $hocr->registerXPathNamespace('x', 'http://www.w3.org/1999/xhtml');
      $words = $hocr->xpath('//x:span[@class="ocrx_word" and @title]');

      foreach ($fields as $field => $snippets) {
        foreach ($snippets as $snippet) {
          // Get matches of tokens contained between the highlighting indicators.
          $matches = array();
          if(preg_match_all($snippet_content_pattern, $snippet, $matches)) {
            // For each match, finds all occurences of the given tokens in the
            // given order...
            foreach ($matches[1] as $match) {
              // Split up into separate tokens.
              $tokens = preg_split('/\s+/', $match);

              // For each match, produce a structure containing the bounding
              //   boxes.
              $spans = islandora_paged_content_get_tokens_from_hocr($words, $tokens);
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

function islandora_paged_content_get_tokens_from_hocr($words, $tokens) {
  $spans = array();

  // XPath can't really do sequences... Instead, we have all the spans holding
  // words given to us.
  $relevant_indicies = array_keys($words, $tokens[0]);
  foreach($relevant_indicies as $index) {
    $count = count($tokens);
    $match = TRUE;
    for ($i = 1; $i < $count && $match; $i++) {
      $match = ((string)$words[$index + $i]) == $tokens[$i];
    }

    if ($match) {
      $spans = array_merge($spans, array_slice($words, $index, $count));
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
 *   - id: The id of the given element.
 *   - class: The class of the given element, 'ocrx_word', 'ocr_line', etc.
 *   - l: The left most point of the element in pixels.
 *   - t: The top most point of the element in pixels.
 *   - r: The right most point of the element in pixels.
 *   - b: The bottom most point of the element in pixels.
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
