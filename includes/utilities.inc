<?php

/**
 * Gets all the pages in the given paged content object.
 *
 * @param FedoraObject $object
 *   The paged content object to fetch the pages from.
 *
 * @return array
 *   All the pages in the given paged content object.
 *
 * @code
 *   array(
 *     'islandora:100' => array(
 *        'label' => 'Page 100',
 *        'page' => '100',
 *        'pid' => 'islandora:100',
 *      )
 *   );
 * @endcode
 */
function islandora_paged_content_get_pages(FedoraObject $object) {
  // @todo Remove old references to old style sequence numbers.
  $query = 'select $object $page $label from <#ri>
            where $object <fedora-rels-ext:isMemberOf> <info:fedora/' . $object->id . '>
            and $object <fedora-model:label> $label
            and ($object <info:islandora/islandora-system:def/paged-item-info#sequenceNumber> $page
            or $object <http://islandora.ca/ontology/relsext#isSequenceNumber> $page)
            order by $page';
  $results = $object->repository->ri->query($query);
  $map = function($o) {
    return array(
      'label' => $o['label']['value'],
      'page' => $o['page']['value'],
      'pid' => $o['object']['value'],
    );
  };
  $pages = array_map($map, $results);
  $get_pid = function($o) {
    return $o['pid'];
  };
  $pids = array_map($get_pid, $pages);
  $pages = count($pids) ? array_combine($pids, $pages) : array();
  $sort = function($a, $b) {
    $a = (is_array($a) && isset($a['page'])) ? $a['page'] : 0;
    $b = (is_array($b) && isset($b['page'])) ? $b['page'] : 0;
    if ($a == $b) {
      return 0;
    }
    return ($a < $b) ? -1 : 1;
  };
  uasort($pages, $sort);
  return $pages;
}

/**
 * Gets the page progression to be used in a viewer from the given paged content object.
 *
 * @param FedoraObject $object
 *   The paged content object to fetch the page progression from.
 *
 * @return string
 *   The page progression of the paged content object:  either left to right 'lr' or
 *   right to left 'rl'.
 */
function islandora_paged_content_get_page_progression(FedoraObject $object) {
  $query = 'select $page_progression from <#ri> where <info:fedora/' . $object->id . '>
            <fedora-rels-ext:hasPageProgression> $page_progression';
  $results = $object->repository->ri->query($query);
  if (count($results) > 0) {
    return $results[0]['page_progression']['value'];
  }
  return 'lr';
}

/**
 * Create/Update a datastream with the given parameters.
 *
 * @param AbstractFedoraObject $object
 *   The object to update.
 * @param string $file
 *   The absolute path to the file to use as content for the datastream.
 * @param string $dsid
 *   The datastream ID.
 * @param string $label
 *   The datastream label.
 * @param string $mimetype
 *   The datastream mimetype, if none given it will be guessed from the file
 *   extension.
 * @param string $control_group
 *   The datastream control group.
 *
 * @return bool
 *   TRUE if successful, FALSE otherwise.
 */
function islandora_paged_content_update_datastream(AbstractFedoraObject $object, $file, $dsid, $label = NULL, $mimetype = NULL, $control_group = 'M', $copy = TRUE) {
  $mime_detector = new MimeDetect();
  if (empty($object[$dsid])) {
    $ds = $object->constructDatastream($dsid, $control_group);
    $object->ingestDatastream($ds);
  }
  else {
    $ds = $object[$dsid];
  }
  $ds->setContentFromFile($file, $copy);
  $ds->label = isset($label) ? $label : $dsid;
  $ds->mimetype = isset($mimetype) ? $mimetype : $mime_detector->getMimetype($file);
  return TRUE;
}

/**
 * Sets a relationship removing any previous relationships.
 *
 * @param FedoraRelationships $relationships
 *   The Fedora relationships to be modified, either RELS-EXT or RELS-INT.
 * @param string $predicate_uri
 *   The predicate URI.
 * @param string $predicate
 *   The predicate.
 * @param string $object
 *   The object.
 * @param bool $literal
 *   TRUE if the object is a literal, FALSE if it is an object.
 */
function islandora_paged_content_set_relationship(FedoraRelationships $relationships, $predicate_uri, $predicate, $object, $literal = FALSE) {
  $relationships->remove($predicate_uri, $predicate, NULL, $literal);
  $relationships->add($predicate_uri, $predicate, $object, $literal);
}

