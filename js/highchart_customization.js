/**
 * @file
 * Chart various highchart customization file.
 */

(function ($, Drupal, H) {

  'use strict';

  var chart_container = {},
    workflow_contributors = {},
    workflow_selected_contributors_count = 0,
    workflow_contributors_count = 0,
    workflow_contributions = {},
    workflow_categories = {},
    no_ui_slider_settings = {},
    workflow_category_colors,
    islandora_object_is_collection = false,
    islandora_object_label = '',
    time_wrapper_id = '',
    chart_date = '',
    chart_title = '',
    main_chart_height = 600;

  Drupal.cwrcCredVizChart = Drupal.cwrcCredVizChart || {};
  Drupal.cwrcCredVizChart.Drilldown = Drupal.cwrcCredVizChart.Drilldown || {};
  Drupal.cwrcCredVizChart.Drillupall = Drupal.cwrcCredVizChart.Drillupall || {};

  Drupal.behaviors.cwrcCredVizChart = {
    attach: function (context, settings) {
      var module_settings = settings.islandora_cwrc_credit_visualization;

      workflow_contributions = module_settings.workflow_contributions;
      workflow_contributors = module_settings.workflow_contributors;
      workflow_selected_contributors_count = module_settings.workflow_selected_contributors_count;
      workflow_contributors_count = module_settings.workflow_contributors_count;
      workflow_categories = module_settings.workflow_categories;
      islandora_object_is_collection = module_settings.is_collection;
      islandora_object_label = module_settings.islandora_object_label;
      time_wrapper_id = module_settings.time_wrapper_id;
      chart_title = module_settings.chart_title;
      no_ui_slider_settings = module_settings.no_ui_slider;

      chart_container = $('.charts-highchart', context);
      chart_container.once('charts-highchart', function () {
        if ($(this).attr('data-chart')) {
          var config = $.parseJSON($(this).attr('data-chart'));

          // Setting the drill up button text.
          H.setOptions({
            lang: {
              drillUpText: Drupal.t('◁ Go back')
            }
          });
          if ($(this).hasClass('charts-highchart--object-credviz-drilldown')) {
            config.chart.events = {
              drilldown: Drupal.cwrcCredVizChart.Drilldown,
              drillupall: Drupal.cwrcCredVizChart.Drillupall
            };

            var user_ids = Object.keys(workflow_contributors);
            main_chart_height = user_ids.length * 20;
            config.chart.height = main_chart_height > 600 ? main_chart_height : 600;

            config.xAxis[0].tickInterval = 1;
            config.xAxis[0].labels.step = 1;
            config.xAxis[0].labels.rotation = 0;
            config.xAxis[0].labels.align = 'left';
            config.xAxis[0].labels.reserveSpace = true;

            config.yAxis[0].allowDecimals = false;
            config.yAxis[0].title.text = Drupal.t('number of contributions');
            config.yAxis[1] = {
              linkedTo: 0,
              opposite: true,
              title: {
                text: Drupal.t('number of contributions')
              }
            };

            // Adding 2 more colors for categories to be used for drilldown.
            var colors = getDataCopy('colors', config);
            colors.push('#ff0000', '#0000ff');
            workflow_category_colors = arrayCombine(Object.keys(workflow_categories), colors);
            delete config.colors;

            // Export config.
            config.exporting = getExportingOptions(main_chart_height > 600 ? main_chart_height : 600);

            // Removing zero columns.
            for (var i = 0; i < config.series.length; i++) {
              var category_data = config.series[i].data,
                new_category_data = [];
              for (var j = 0; j < category_data.length; j++) {
                if (category_data[j].y > 0) {
                  new_category_data.push(category_data[j]);
                }
              }
              config.series[i].data = new_category_data;
            }
            config.title.useHTML = true;

            // Chart current date.
            var date = new Date(),
              date_options = {
                month: 'long',
                day: 'numeric',
                year: '2-digit',
                hour: '2-digit',
                minute: '2-digit',
                timeZoneName: 'short'
              };
            chart_date = date.toLocaleDateString([navigator.language, 'en-US'], date_options);
          }

          $(this).highcharts(config);
        }
      });
    }
  };

  Drupal.cwrcCredVizChart.Drilldown = function (e) {
    if (!e.seriesOptions && !chart_container.hasClass('drilling-down')) {
      var chart = this,
        user_id = e.point.user_id,
        category_id = '<all>',
        point = e.point;

      // User clicked one of the stacked color.
      if (e.originalEvent.hasOwnProperty('point')) {
        point = e.originalEvent.point;
        user_id = point.user_id;
        category_id = point.category_id;
      }

      // Show the loading message.
      chart.showLoading('Loading data ...');
      chart.hideLoading();
      var series = getDrilldownData(user_id, category_id),
        height = 0,
        default_height = 600;

      if (category_id !== '<all>') {
        // Updating the drilldown object.
        chart.addSeriesAsDrilldown(point, series);
        height = series.data.length * 15;
      }
      else {
        var i;
        for (i = 0; i < series.length; i++) {
          chart.addSingleSeriesAsDrilldown(point, series[i]);
          height = height + series[i].data.length * 15;
        }
        chart.applyDrilldown();
      }

      var config_update = {
        title: {
          text: Drupal.t('<span style="font-weight:bold">Credit Visualization</span> of contributions by <span style="font-weight:bold">@user</span> to <span style="font-weight:bold">@label</span> (as of <span id="@id">@date</span>)', {
            '@user': workflow_contributors[user_id],
            '@label': islandora_object_label,
            '@type': islandora_object_is_collection ? 'collection' : 'document',
            '@id': time_wrapper_id,
            '@date': chart_date
          })
        },
        subtitle: {
          text: ''
        },
        chart: {
          // inverted: true, // Inverting the chart to make it look like a bar chart type.
          height: default_height > height ? default_height : height // Dynamic chart height based on the documents.
        },
        xAxis: {
          title: { text: Drupal.t('Item(s)') },
          labels: {
            align: 'left',
            reserveSpace: true,
            rotation: 0,
            step: 1
          }
        },
        exporting: getExportingOptions(default_height > height ? default_height : height),
      };

      chart.update(config_update);
      // To prevent the drill down event running down more that once.
      chart_container.addClass('drilling-down');
    }
  };

  Drupal.cwrcCredVizChart.Drillupall = function (e) {
    var chart = this;

    var config_update = {
        title: {
          text: chart_title
        },
        subtitle: {
          text: Drupal.t('Click on the columns to view user contribution(s) by document')
        },
        chart: {
          height: main_chart_height > 600 ? main_chart_height : 600,
        },
        xAxis: {
          title: { text: Drupal.t('contributor(s)') }
        }
      };

    chart.update(config_update);
  };

  function getDrilldownData(user_id, category_id) {
    var series,
      data,
      username = workflow_contributors[user_id];
    if (workflow_contributions.hasOwnProperty(user_id)) {
      var user_contributions = getDataCopy(user_id, workflow_contributions);
      if (category_id !== '<all>' && workflow_categories.hasOwnProperty(category_id)) {
        var category_name = workflow_categories[category_id];
        data = getDataCopy(category_id, user_contributions.categories).documents;
        var filtered_data = getFilteredData(data);
        series = {
          name: workflow_categories[category_id],
          tooltip: {
            pointFormat: '<b>' + username + '</b> - ' + category_name + ' (<b>{point.y}</b>)'
          },
          data: Object.keys(filtered_data).map(function (pid) {
            return {
              name: filtered_data[pid].label,
              y: filtered_data[pid].count
            }
          })
        };
      }
      else {
        series = [];
        for (var category in user_contributions.categories) {
          if (user_contributions.categories.hasOwnProperty(category)) {
            data = getDataCopy(category, user_contributions.categories).documents;
            var filtered_data = getFilteredData(data);
            series.push({
              name: workflow_categories[category],
              color: workflow_category_colors[category],
              tooltip: {
                pointFormat: '<b>' + username + '</b> - ' + workflow_categories[category] + ' (<b>{point.y}</b>)'
              },
              data: Object.keys(filtered_data).map(function (pid) { return {
                  name: filtered_data[pid].label,
                  y: filtered_data[pid].count
                }
              })
            });
          }
        }
      }
    }

    return series;
  }

  function getDataCopy(property, data) {
    return data[property];
  }

  function getFilteredData(data) {
    var filtered_data = {};
    for (var document_key in data) {
      if (data.hasOwnProperty(document_key) && data[document_key].count > 0) {
        filtered_data[document_key] = data[document_key];
      }
    }
    return filtered_data;
  }

  function arrayCombine(keys, values) {
    var result = [];
    for (var i = 0; i < keys.length; i++) {
      result[keys[i]] = values[i];
    }
    return result;
  }

  function getExportingOptions(height) {
    return {
      allowHTML: true,
      chartOptions: {
        chart: {
          height: height + 50,
          marginBottom: 150,
          events: {
            load: function () {
              var renderer = this.renderer,
                title = this.title,
                sub_title = this.subtitle,
                legend_title_text = this.legend.title.element.querySelector('.highcharts-category-contribution-label-help-text'),
                x = 13,
                chart_height = this.userOptions.chart.height,
                text_y = chart_height - 90,
                text_line_y = chart_height - 95,
                text_width = this.userOptions.chart.width,
                url = location.protocol + '//' + location.host + location.pathname;

              renderer.path(['M', x, text_line_y, 'L',  600, text_line_y, 'Z']).attr({
                stroke: 'black',
                'stroke-width': 1
              }).add();

              renderer.label(url.replace('%3A', ':'), x, text_y)
                .css({width: text_width + 'px'})
                .add();

              // Hiding subtitle.
              sub_title.hide();

              // Legend title.
              if (legend_title_text) {
                legend_title_text.textContent = Drupal.t('(Grey categories are filtered)');
              }

              // Adding inline styles for the table in the header.
              var title_element = title.element;
              if (title_element) {
                // Apply styles inline because stylesheets are not passed to the exported SVG.
                H.css(title_element, {
                  'font-size': '20px'
                });
                H.css(title_element.querySelector('table'), {
                  'border-collapse': 'separate',
                  'border-spacing': 0,
                  background: 'white',
                  'min-width': '100%',
                  'font-family': 'sans-serif',
                  'font-size': '15px',
                  'margin-top': '15px'
                });
                [].forEach.call(title_element.querySelectorAll('th'), function (elem) {
                  H.css(elem, {
                    'line-height': 1,
                    padding: '5px 5px',
                    background: '#fff',
                    color: '#333',
                    'font-weight': 900,
                    'text-align': 'left',
                  });
                });
                [].forEach.call(title_element.querySelectorAll('table thead th'), function (elem) {
                  H.css(elem, {
                    'border-top': '1px solid #eee',
                    'border-left': '1px solid #eee',
                    'border-bottom': '3px solid #ddd',
                  });
                });
                [].forEach.call(title_element.querySelectorAll('table tbody th'), function (elem) {
                  H.css(elem, {
                    'border-bottom': '1px solid #eee',
                    'border-left': '1px solid #eee',
                  });
                });
                [].forEach.call(title_element.querySelectorAll('td'), function (elem) {
                  H.css(elem, {
                    'line-height': 1,
                    padding: '5px 5px',
                    'border-bottom': '1px solid #eee',
                    'border-left': '1px solid #eee',
                    'vertical-align': 'middle',
                  });
                });
                [].forEach.call(title_element.querySelectorAll('tr th:last-of-type, tr td:last-of-type'), function (elem) {
                  H.css(elem, {
                    'border-right': '1px solid #eee',
                  });
                });
                if (title_element.querySelector('caption > span')) {
                  H.css(title_element.querySelector('caption > span'), {
                    'line-height': 1,
                    padding: '5px 0',
                    'padding-left': '5px',
                    'padding-right': '5px',
                    'border-bottom': '1px solid #eee',
                    display: 'block',
                    background: '#fafafa',
                    'text-align': 'left',
                    'border-right': '1px solid #eee',
                    'border-left': '1px solid #eee',
                  });
              }
                H.css(title_element.querySelector('caption'), {
                  'border-bottom': 'none',
                  background: 'transparent',
                  'caption-side': 'bottom',
                  padding: 0,
                });
              }
            }
          }
        }
      },
      tableCaption: false,
      buttons: {
        contextButton: {
          menuItems: [
            'viewFullscreen',
            'printChart',
            'separator',
            'downloadPNG',
            'downloadJPEG',
            'downloadPDF',
            'separator',
            'viewData',
          ]
        }
      }
    };
  }

  H.Chart.prototype.callbacks.push(function (chart) {
    // This event get triggered after the drilldown, removing the drilling-down
    // class here.
    H.addEvent(chart.container, 'click', function (e) {
      if (chart_container.hasClass('drilling-down')) {
        chart_container.removeClass('drilling-down');
      }
    });
  });
})(jQuery, Drupal, Highcharts);
