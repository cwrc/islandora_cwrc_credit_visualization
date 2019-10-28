/**
 * @file
 * Add the no ui customization and some date filter handler.
 */

(function ($, Drupal) {

  Drupal.behaviors.cwrcCredVizChartDateHandler = {
    attach: function (context, settings) {
      var module_settings = settings.islandora_cwrc_credit_visualization,
        no_ui_slider_settings = module_settings.no_ui_slider,
        dateFilterSlider = document.getElementById(no_ui_slider_settings.placeholder_id),
        dateFilterDisplays = [
          document.getElementById('filter-date-range-start'),
          document.getElementById('filter-date-range-end')
        ],
        dateMinInput = document.getElementsByName("date_range[min]")[0],
        dateMaxInput = document.getElementsByName("date_range[max]")[0],
        time_wrapper = $('.highcharts-title > tspan:nth-child(4)', context);

      if (time_wrapper.length) {
        var date = new Date(),
          date_options = {
            month: 'long',
            day: 'numeric',
            year: '2-digit',
            hour: '2-digit',
            minute: '2-digit',
            timeZoneName: 'short'
          };

        time_wrapper.html(date.toLocaleDateString([navigator.language, 'en-US'], date_options));
      }

      // Instantiate the no ui slider for date range filter.
      noUiSlider.create(dateFilterSlider, {
        connect: true,
        // No closer than 1 together.
        margin: 1,
        // Snap to 1-unit increments.
        step: 1,
        range: {
          min: Drupal.islandoraCwrcCreditVisualization.timestamp(no_ui_slider_settings.range.min),
          max: Drupal.islandoraCwrcCreditVisualization.timestamp(no_ui_slider_settings.range.max)
        },
        start: [
          Drupal.islandoraCwrcCreditVisualization.timestamp(no_ui_slider_settings.start.begin),
          Drupal.islandoraCwrcCreditVisualization.timestamp(no_ui_slider_settings.start.end),
        ],
        format: wNumb({
          decimals: 0
        }),
        pips: {
          mode: 'positions',
          values: [0, 33, 66, 100],
          density: 1,
          format: {
            to: function (value) {
              var date = new Date(value);
              return Drupal.islandoraCwrcCreditVisualization.formatDate(date);
            },
            from: function (value) {
              return value.getTime();
            }
          }
        }
      });
      dateFilterSlider.noUiSlider.on('update', function (values, handle) {
        var value = values[handle];
        // Update the date display for the user.
        dateFilterDisplays[handle].innerHTML = Drupal.islandoraCwrcCreditVisualization.formatDate(new Date(+value));
        // Update hidden min and max value.
        var value_input = value / 1000;
        if (handle) {
          dateMaxInput.value = value_input.toFixed(0);
        }
        else {
          dateMinInput.value = value_input.toFixed(0);
        }
      });
    }
  };
})(jQuery, Drupal);
