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
 *   -parent (Optional): A string specifying the content model of the parent.
 *   Not needed if it's belonging to a collection.
 *   -predicate: A string specifying the predicate of the relationship to
 *   the parent.
 *   -children: An array containing arrays of keyed content model children.
 */
function hook_islandora_paged_content_content_model_registry() {
  return array(
    'somecmodel' => array(
      'parent' => 'islandora:bookCModel',
      'predicate' => 'isMemberOfCollection',
      'children' => array(
        'somecmodel' => array(),
      ),
    ),
  );
}
