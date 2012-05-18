<?php

/*
 * contains methods to search solr and display results.  depends on Apache_Solr_Php client.
 */


/**
 * Extention of IslandoraSolrResults
 * This overrides the printResults function to provide customized display.
 */
module_load_include('inc', 'islandora_solr_search', 'IslandoraSolrResults');

class CairnSolrResults extends IslandoraSolrResults {

  static $facetSeparator = '~'; //used to separate facets in url
  static $slashReplacement = '~slsh~'; // a pattern used to replace / in url's the slash breaks drupal clean url's
  public static $SEARCH_CLASS_ADVANCED_SEARCH_NUMBER_FIELDS = 5;

  function CairnSolrResults() {
    module_load_include('php', 'islandora_solr_search', 'Solr/Service');
  }

  /**
   *
   * @param $results
   *   A solr resultset object.
   *
   * @return
   *   html output for the resultset. Note: we currently create this
   *   output manually, should refactor to use drupal forms api.
   */
  function printResults($results) {
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');
    module_load_include('inc', 'citeproc', 'CiteProcJSBibliography');
    module_load_include('inc', 'scholar', '/citation/bibliography/BibliographyForm');
    module_load_include('inc', 'scholar', '/citation/bibliography/CitationForm');
    module_load_include('inc', 'scholar', 'Coins');
    module_load_include('inc', 'islandora_bibliography', 'Bibliography');
    drupal_add_css(drupal_get_path('module', 'islandora_bibliography') . '/css/export.css');
    drupal_add_css(drupal_get_path('module', 'scholar') . '/scholar.css');

    global $base_url;
    $path = drupal_get_path('module', 'scholar') . '/images';

    $institutions = array(
      'msvu' => "Mount Saint Vincent",
      'upei' => "U.P.E.I.",
      'acadiau' => "Acadia University",
      'ast' => "Atlantic School of Theology",
      'cbu' => "Cape Breton University",
      'dal' => "Dalhousie University",
      'mta' => 'Mount Allison Universiyy',
      'nscad' => 'NSCAD University',
      'nscc' => 'Nova Scotia Community College',
      'smu' => "Saint Mary's University",
      'mun' => "Memorial University of Newfoundland",
      'nsac' => 'Nova Scotia Agricultural College',
      'stfx' => 'St. Francis Xavier University',
      'ukings' => "University of King's College",
      'unbsj' => 'UNB Saint John',
      'unb' => 'University of New Brunswick',
      'usaintanne' => 'Université Sainte-Anne',
      'umoncton' => 'Université de Moncton',
    );
    $recordStart = (int) $results->response->start;
    if (empty($results)) {
      return "no results";
    }
    $rawResponse = $results->getRawResponse();
    $responseArray = json_decode($rawResponse, true);
    $docs = $responseArray['response']['docs'];
    $csl = 'default';
    foreach ($docs as $doc) {

    }
    foreach ($docs as $doc) {

      $PID = $doc['PID'];

      $style = citeproc_style($csl);
      $bibliography = new CiteProcJSBibliography($style);
      $mods = islandora_bibilgraphy_form_get_mods($PID);

      if ($mods) {
        $ref = citeproc_citation_from_mods($mods);
        $bibliography->addCitation($ref);
      }
      $pid_parts = explode(':', $PID);
      $institution = $institutions[$pid_parts[0]];
      $collection_key = "";
      if (isset($doc['rels.isMemberOfCollection'])) {
        $collection_key = 'rels.isMemberOfCollection';
      }
      if (isset($doc['rels.isMemberOf'])) {
        $collection_key = 'rels.isMemberOf';
      }
      if (isset($doc['rels.fedora:isMemberOfCollection'])) {
        $collection_key = 'rels.fedora:isMemberOfCollection';
      }
      $collection_pid = $doc[$collection_key][0];
      $collection_pid = preg_replace('/info:fedora\//', '', $collection_pid);
      if ($collection_pid) {
        $item = new Fedora_Item($collection_pid);
        if ($item->exists()) {
          $collection_name = $item->objectProfile->objLabel;
          $collection_tag = "<a href='http://" . $pid_parts[0] . ".cairnrepo.org/fedora/repository/$collection_pid'>$collection_name</a>";
        }
      }
      if (!$collection_tag) {
        $collection_tag = "Collection not Found";
      }
      $title = $doc['dc.title'][0];
      $thumb_tag = "<a href='http://" . $pid_parts[0] . ".cairnrepo.org/fedora/repository/$PID'>";
      $thumb_tag .= "<img src='http://" . $pid_parts[0] . ".cairnrepo.org/fedora/repository/$PID/TN' alt='No image available' /></a>";
      $title_tag = "<a href='http://" . $pid_parts[0] . ".cairnrepo.org/fedora/repository/$PID'>$title</a>";
      $rows = array();
      $rows[] = array('data' => array(array('data' => $thumb_tag, 'class' => "solr-tn-med", 'rowspan' => 4), array('data' => $title_tag, 'class' => ' solr-mods-values')), 'class' => 'row-first');
      $rows[] = array(array('data' => $collection_tag, 'class' => 'solr-mods-values'));
      $rows[] = array('data' => array(array('data' => $institution, 'class' => 'solr-mods-values')));
      $content_model = $doc['rels.fedora-model:hasModel'][0];
      if (!$content_model) {
        $content_model = $doc['rels.hasModel'][0];
      }


      $bibliography_text = Bibliography::Exists($PID) ? 'Remove Reference from List' : 'Add Reference to List';
      $object_title = '<div class="scholar_search_title">' . l($title, 'fedora/repository/' . $PID) . '</div>';

      $bookmark = '<div class="scholar_search_bookmark">' . drupal_get_form('islandora_bibliography_citation_search_form', $PID) . '</div><div class="scholar_search_bookmark_text">' . $bibliography_text . '</div>';
      $related_img = $path . '/reload.png';
      $related = '<div class="scholar_search_related">' . l('<img src="' . $related_img . '"/> Related articles', 'islandora/solr/search/' . str_replace(' ', ' OR ', $title), array('html' => TRUE)) . '</div>';

      $citation_data = $object_title;
      if ($content_model != 'info:fedora/islandora:pageCModel') {
        $citation_data .= '<div class="scholar_search_citation">';
        $citation_data .= $bibliography->render();
        $citation_data .= '</div>';
        $citation_data .= '<div class="scholar_search_citation">';
        $citation_data .= $abstract;
        $citation_data .= '</div>';
        $citation_data .= $pdf_result;
        $citation_data .= $bookmark;
        //$citation_data .= $related;
      }
      $rows[] = array('data' => array('data' => $citation_data), 'class' => 'row-last');

      $output .= theme_table(null, $rows);
    }
    $uri = request_uri();
    $base_path = base_path();
    $redirect = preg_replace("/$base_path/", '', $uri);
    $redirect = preg_replace('/\/\//', '/', $redirect);

    $_SESSION['search_url'] = request_uri();

    $_SESSION['search_url'] = $redirect;
    return '<div class ="islandora-solr-search">' . $output . '</div>';
  }

  /**
   * Function: displayFacets
   *
   * Description: Displays basic facets based on an apache solr query response,
   *   as contained with the IslandoraSolrQueryProcessor.
   *
   * @global string $base_url
   * @param IslandoraSolrQueryProcessor $solrQueryProcessor
   * @return string
   */
  function displayCairnBookFacets($solrQueryProcessor) {
    global $base_url;
    $default_display = variable_get('islandora_solr_primary_display', 'default');
    $facet_output = $output = '';
    $islandora_facets = isset($solrQueryProcessor->solrResult->facet_counts->facet_fields) ? $solrQueryProcessor->solrResult->facet_counts->facet_fields : array();
    if (empty($islandora_facets)) {
      return $output; //no facets to show
    }

    // set show more variable
    $show_more = FALSE;

    // set display
    $display = $solrQueryProcessor->display;

    if ($display == $default_display) {
      $display_param = '';
    }
    else {
      $display_param = $display;
    }
//var_dump($solrQueryProcessor);
    foreach ($islandora_facets as $key => $field) {
      $list_items = array();
      $list_type = "ul";
      $list_title = NULL;

      $test = get_object_vars($field); //get the number of fields if there aren't any don't show the key
      if (count($test) > 0) {
        $facet_count = 0;
        unset($normal_facet_output);

        foreach ($field as $name => $number) {
          $this->_addFacets($key, $name, $number, $solrQueryProcessor, $facet_count, $list_items, $display_param);
        }

        if (count($list_items) > 0) {
          $facet_output .='<div class="islandora_solr_search_facet">';

          // shown limit
          $shown_limit = variable_get('islandora_solr_search_block_facet_shown_limit', 0);
          $facetlimit = variable_get('islandora_solr_search_block_facet_limit', '12');
          $facetMinCount = variable_get('islandora_solr_search_block_facet_min_count', '2');

          $list_title = $this->facetFieldArray[$key];
          $list_type = "ul";
          $list_attributes = array(
            'class' => implode(' ', array(
              'islandora_solr_search_facet_list',
              'facet_list'))
          );

          // split the array if shown_limit is between the min and max limits and if the total is larger than the shown_limit.
          $split_facet = $facet_count > $shown_limit AND
              $shown_limit < $facetlimit AND
              $shown_limit > $facetMinCount;

          if ($split_facet) {
            $show_more = TRUE; //There exists a split array...
            $list_items_hidden = array_slice($list_items, $shown_limit);
            $list_items = array_slice($list_items, 0, $shown_limit);
          }

          $facet_output .= theme_item_list($list_items, $list_title, $list_type, $list_attributes);

          // render the hidden facets
          if ($split_facet) {
            $facet_output .= '<a href="#" class="shown-toggle">' . t('Show more') . '</a>';
            $facet_output .= '<a href="#" class="shown-toggle hidden">' . t('Show less') . '</a>';
            $facet_output .= '<div class="toggle-wrapper hidden">';
            $facet_output .= theme_item_list($list_items_hidden, NULL, $list_type, $list_attributes);
            $facet_output .= '</div>';
          }

          $facet_output .='</div>'; //div.islandora_solr_search_facet
        }
      }
    }

    if ($show_more) {
      // if there is at least one occurence of a split array, we add js to the page.
      drupal_add_js(drupal_get_path('module', 'islandora_solr_search') . '/js/islandora_solr_search_shown_limit.js');
    }

    //////////////////////////////////////////////////////////////////////////////////////
    // as we add additional facets, we're repeatedly URL-encoding old facet             //
    // strings. when we double-encode quotation marks they're incomprehensible to solr. //
    // This is a quick workaround:                                                      //
    //////////////////////////////////////////////////////////////////////////////////////
    $facet_output = str_replace('%2B', '%252B', $facet_output);
    if (!empty($facet_output)) {
      $output .='<div class="islandora_solr_search_facet_wrap">';
      $output .= $facet_output;
      $output .= '</div>';
    }

    return $output;
  }

}