<?php

/**
 * @file
 * This file documents all available hook functions to manipulate data.
 */

/**
 * Identifies content models whose children are paged content.
 *
 * @return array
 *   An array mapping the parent paged content model to an array of its child
 *   pages.
 */
function hook_islandora_paged_content_content_model_registry() {
  return array(
    'somecmodel' => array(
      'somepagecmodel',
    ),
  );
}
