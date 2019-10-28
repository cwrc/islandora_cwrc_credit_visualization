/**
 * @file
 * Add the no ui customization and some date filter handler.
 */

(function ($, Drupal) {

  Drupal.behaviors.cwrcCredVizChartCheckboxesHandler = {
    attach: function (context, settings) {
      var module_settings = settings.islandora_cwrc_credit_visualization,
        users_checkboxes = '#' + module_settings.filters_wrapper_id + ' .form-item-users input',
        selAllNoneWrapper = [$('#edit-users-select-all-toggle-wrapper', context), $('#edit-document-pids-select-all-toggle-wrapper', context)],
        selAll = Drupal.t('Select All'),
        selNone = Drupal.t('Select None');

      $.each(selAllNoneWrapper, function (i, toggleWrapper) {
        var allChecked = true,
          checkboxes = $(this).closest('.form-type-checkboxes', context).find('.form-checkboxes .form-checkbox');

        if ($(toggleWrapper).length) {
          checkboxes.each(function (i, item) {
            if (!$(item).is(':checked')) {
              allChecked = false;
              return;
            }
            $(item).on('change', function (e) {
              var link_toggle = $(this).closest('.form-type-checkboxes', context).find('label .checkboxes-select-all-none-toggle');
              checkboxes.not(':checked').length ? link_toggle.html(selAll) : link_toggle.html(selNone);
            });
          });
          if (allChecked) {
            $(toggleWrapper).append(getToggleLink(selNone));
          }
          else {
            $(toggleWrapper).append(getToggleLink(selAll));
          }
        }
      });

      // Top actions fake buttons to trigger submit/reset using them.
      $('[id^=top-actions-]').on('click touch', function (e) {
        var button = $(this).data('button');
        e.preventDefault();
        if (button && (button === 'reset' || button === 'apply')) {
          $('[id^=edit-' + button + ']', '#cwrc-islandora-credit-visualization-highchart-filters-wrappers', context).trigger('click');
        }
      });

      function filterByUsers(chart, series) {
        var checked = [];
        // Getting checked users.
        $.each($(users_checkboxes, context), function (i, user) {
          if ($(this).is(':checked')) {
            checked.push($(this).attr('name'));
          }
        });

        // var newCategories = [];
        $.each(series, function (i, ser) {
          var newData = [];
          $.each(ser.data, function (j, point) {
            // console.log(point);
            var input_name = 'users[' + point.user_id + ']';
            if ($.inArray(input_name, checked) != -1) {
              newData.push(point);
              // newCategories.push(point.name);
            }
          });
          chart.series[ser.index].setData(newData);
        });
        // chart.xAxis[0].setCategories(newCategories, false);
        // chart.redraw();
      }

      function getToggleLink(link_text) {
        var toggleLink = $('<a class="checkboxes-select-all-none-toggle" href="#">' + link_text + '</a>'),
          selNoneWarning = Drupal.t('You must keep at least one item selected to load more items. Deselect after loading if desired.');

        toggleLink.click(function (event) {
          // Don't actually follow the link...
          event.preventDefault();
          event.stopPropagation();

          var noneSelected = selAll === $(this).text();
          $(this).html(noneSelected ? selNone : selAll);
          $(this).attr('title', noneSelected ? selNoneWarning : '');

          var checkboxes = $(this).closest('.form-type-checkboxes', context).find('.form-checkboxes .form-checkbox');
          checkboxes.each(function (index, item) {
            $(item).prop('checked', noneSelected);
          });
        });
        return toggleLink;
      }
    }
  };
})(jQuery, Drupal);
