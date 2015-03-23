<?php

/**
 * @file
 * This file documents all available hook functions to manipulate data.
 */

/**
 * Identifies content models whose children are paged content.
 *
 * @note
 *   The children content model PIDs currently map to an empty array as there
 *   may be room for expansion in the future.
 * @return array
 *   An array mapping the parent paged content model to an array of keyed
 *   content model arrays containing:
 *   - parents: An associative array mapping applicable content models to
 *     predicates by which the current entry might be related.
 *     Not strictly needed if it belongs to a collection.
 *   - children: An array containing arrays of keyed content model children.
 */
function hook_islandora_paged_content_content_model_registry() {
  return array(
    'somecmodel' => array(
      'parents' => array(
        'islandora:bookCModel' => 'isMemberOfCollection',
      ),
      'children' => array(
        'somecmodel' => array(),
      ),
    ),
  );
}
