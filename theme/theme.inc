<?php

/**
 * @file
 * Theme hooks.
 */

require_once dirname(__FILE__) . '/../includes/utilities.inc';

/**
 * Theme the page navigator.
 */
function theme_islandora_paged_content_page_navigator(array $variables) {
  $object = $variables['object'];
  $results = $object->relationships->get(ISLANDORA_RELS_EXT_URI, 'isPageOf');
  $parent = islandora_object_load($results[0]['object']['value']);
  $pages = islandora_paged_content_get_pages($parent);
  $pages = array_keys($pages);
  $index = array_search($object->id, $pages);
  $previous_page = isset($pages[$index - 1]) ? $pages[$index - 1] : NULL;
  $next_page = isset($pages[$index + 1]) ? $pages[$index + 1] : NULL;
  $links = array();
  if ($previous_page) {
    $links[] = array(
      'title' => t('Prev'),
      'href' => url("islandora/object/{$previous_page}", array('absolute' => TRUE)),
    );
  }
  $links[] = array(
    'title' => '<strong>' . t('Page') . '</strong>',
    'html' => TRUE,
  );
  if ($next_page) {
    $links[] = array(
      'title' => t('Next'),
      'href' => url("islandora/object/{$next_page}", array('absolute' => TRUE)),
    );
  }
  $links[] = array(
    'title' => t('All Pages'),
    'href' => url("islandora/object/{$parent->id}", array('absolute' => TRUE)),
  );
  return theme('links', array(
      'links' => $links,
      'attributes' => array('class' => array('links', 'inline'))));
}

/**
 * Implements template_preprocess_theme().
 */
function template_preprocess_swaptable(array &$variables) {
  element_set_attributes($variables['element'], array('id', 'name'));
  $table = &$variables['element'];
  $items = element_children($table['#rows']);
  $pagers = swaptable_pagers($table['#pager'], count($items));
  $variables['left_table'] = swaptable_table('left', $table, $pagers[0]);
  $variables['right_table'] = swaptable_table('right', $table, $pagers[1]);
  $base = $table['#id'];
  $settings['swapTable'][$base] = array(
    'left' => "{$base}-left",
    'right' => "{$base}-right",
    'items' => $items,
  );
  drupal_add_js($settings, 'setting');
}

/**
 * Builds the descriptions of both pagers for use in the swaptable.
 *
 * @param array $pager
 *   The general description of the pagers.
 * @param int $total
 *   The total number of items to page through.
 *
 * @return array
 *   The description of both pagers for use in the swaptable.
 */
function swaptable_pagers(array $pager, $total) {
  extract($pager);
  return array(
    0 => swaptable_pager($element[0], $tags, $quantity, $limit, $total),
    1 => swaptable_pager($element[1], $tags, $quantity, $limit, $total),
  );
}

/**
 * Builds a description of a pager for use in the swaptable.
 *
 * @param int $element
 *   The unique id for the pager.
 * @param array $tags
 *   The tags definition to use when rendering the pager.
 * @param int $quantity
 *   The number of pages to display as links.
 * @param int $limit
 *   The number of items to show on a page.
 * @param int $total
 *   The total number of items to page through.
 *
 * @return array
 *   The pager definition for use in the swaptable.
 */
function swaptable_pager($element, $tags, $quantity, $limit, $total) {
  $page = pager_default_initialize($total, $limit, $element);
  return array(
    'page' => $page,
    'limit' => $limit,
    'element' => $element,
    'pager' => theme('pager', array(
        'tags' => $tags,
        'element' => $element,
        'quantity' => $quantity,
      )),
  );
}

/**
 * Prepares the rows to be rendered in a table.
 *
 * @param string $position
 *   The position of the table either 'left' or 'right'.
 * @param array $items
 *   The description of each row of the table where the keys are the unique
 *   identifers for each row.
 * @param array $pager
 *   The description of the pager to attach to this table.
 *
 * @return array
 *   The rows definition for the table.
 */
function swaptable_table_rows($position, array &$items, array $pager) {
  $rows = array();
  // Once pager support is enabled uncomment the line below.
  // Code sniffer was refusing to ignore the commented code here.  Please see
  // d251150a273ce3f4a52a3948ecb8ea235433cb8d to retrieve it.
  $count = 1;
  foreach ($items as $id => &$row) {
    $data = array();
    $data[] = array(
      'data' => $count,
      'class' => array('ordering'),
    );
    $data[] = array(
      'data' => $count,
      'class' => array('original-ordering', 'swaptable-hide'),
    );
    foreach (element_children($row) as $j) {
      $cell = &$row[$j];
      // @todo Make label into links?
      $data[] = array(
        'data' => empty($cell['#printed']) ? drupal_render($cell) : $cell['#children'],
        'class' => array($j),
      );
    }
    $rows[] = array(
      'id' => str_replace(':', '-', "{$position}-{$id}"),
      'class' => array(),
      'data' => $data,
    );
    $count++;
  }
  return $rows;
}

/**
 * Renders a single swap table where $position, denotes 'left' or 'right'.
 *
 * @param string $position
 *   The position of the table either 'left' or 'right'.
 * @param array $table
 *   The description of the table.
 * @param array $pager
 *   The description of the pager to attach to this table.
 *
 * @return string
 *   The rendered table.
 */
function swaptable_table($position, array $table, array $pager) {
  // To get pagers to render just add append "$pager['pager']" to the
  // the returned value.
  $table['#attributes']['id'] = "{$table['#id']}-{$position}";
  $table['#attributes']['name'] = $position;
  $table['#attributes']['valign'] = 'top';
  $header = array(
    array(
      'data' => t('#'),
      'class' => array('ordering'),
    ),
    array(
      'data' => t('*#'),
      'class' => array('original-ordering', 'swaptable-hide'),
    ));
  return theme('table', array(
      'header' => array_merge($header, $table['#header']),
      'rows' => swaptable_table_rows($position, $table['#rows'], $pager),
      'attributes' => $table['#attributes']));
}
