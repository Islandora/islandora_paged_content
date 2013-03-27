<?php

/**
 * @file
 * Theme hooks.
 */

require_once dirname(__FILE__) . '/../includes/utilities.inc';

/**
 * Implements theme_form_id().
 */
function theme_islandora_paged_content_manage_pages_sequences_form(array $variables) {
  $form = $variables['form'];
  $rows = array();
  $weight_class = 'islandora-page-weight';
  $table = &$form['table'];
  foreach (element_children($table['rows']) as $id) {
    $row = &$table['rows'][$id];
    $row['weight']['#attributes'] = array('class' => array($weight_class));
    $rows[] = array(
      'data' => array(
        drupal_render($row['pid']),
        drupal_render($row['label']),
        drupal_render($row['weight']),
      ),
      'class' => array('draggable'),
    );
  }
  $table_id = 'islandora-reorder-pages-table';
  $output = theme('table', array(
              'header' => $table['#header'],
              'rows' => $rows,
              'attributes' => array('id' => $table_id)));
  $output .= drupal_render_children($form);
  drupal_add_tabledrag($table_id, 'order', 'sibling', $weight_class);
  return $output;
}

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
