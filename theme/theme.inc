<?php

/**
 * @file
 * Theme hooks.
 */

/**
 * Implements theme_form_id().
 */
function theme_islandora_paged_content_manage_pages_sequences_form($variables) {
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
