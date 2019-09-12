/**
 * @file
 * Chart various highchart customization file.
 */

(function ($, Drupal, H) {

  'use strict';

  var chart_container = {},
    workflow_contributors = {},
    workflow_contributions = {},
    workflow_categories = {},
    workflow_category_colors,
    rotate_xaxis_labels = false,
    islandora_object_is_collection = false,
    islandora_object_label = '',
    time_wrapper_id = '',
    chart_date = '',
    main_chart_height = 600;

  Drupal.cwrcCredVizChart = Drupal.cwrcCredVizChart || {};
  Drupal.cwrcCredVizChart.Drilldown = Drupal.cwrcCredVizChart.Drilldown || {};
  Drupal.cwrcCredVizChart.Drillupall = Drupal.cwrcCredVizChart.Drillupall || {};

  Drupal.behaviors.cwrcCredVizChart = {
    attach: function (context, settings) {
      var module_settings = settings.islandora_cwrc_credit_visualization;

      workflow_contributions = module_settings.workflow_contributions;
      workflow_contributors = module_settings.workflow_contributors;
      workflow_categories = module_settings.workflow_categories;
      rotate_xaxis_labels = module_settings.rotate_xaxis_labels;
      islandora_object_is_collection = module_settings.is_collection;
      islandora_object_label = module_settings.islandora_object_label;
      time_wrapper_id = module_settings.time_wrapper_id;

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
            config.yAxis[0].title.text = Drupal.t('contributions count');
            config.yAxis[1] = {
              linkedTo: 0,
              opposite: true,
              title: {
                text: Drupal.t('contributions count')
              }
            };

            // Adding 2 more colors for categories to be used for drilldown.
            var colors = getDataCopy('colors', config);
            colors.push('#ff0000', '#0000ff');
            workflow_category_colors = arrayCombine(Object.keys(workflow_categories), colors);
            delete config.colors;
            // console.log(workflow_category_colors);

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
          text: Drupal.t('Contributions by <b>@user</b> to <b>@label</b> (as of <span id="@id">@date</span>)', {
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
        }
      };

      chart.update(config_update);
      // To prevent the drill down event running down more that once.
      chart_container.addClass('drilling-down');
    }
  };

  Drupal.cwrcCredVizChart.Drillupall = function (e) {
    var chart = this,
      config_update = {
        title: {
          text: Drupal.t('Contributions to <b>@label</b> @type (as of <span id="@id">@date</span>)', {
            '@label': islandora_object_label,
            '@type': islandora_object_is_collection ? 'collection' : 'document',
            '@id': time_wrapper_id,
            '@date': chart_date
          })
        },
        subtitle: {
          text: Drupal.t('Click on the columns to view user contribution(s) by document')
        },
        chart: {
          // inverted: false,
          height: main_chart_height > 600 ? main_chart_height : 600,
        },
        xAxis: {
          title: { text: Drupal.t('contributor(s)') }
        }
      };

    // if (rotate_xaxis_labels) {
    //   config_update.xAxis.labels = {
    //     align: undefined,
    //     rotation: 90,
    //     step: 1
    //   };
    // }
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
        series = {
          name: workflow_categories[category_id],
          tooltip: {
            pointFormat: '<b>' + username + '</b> - ' + category_name + ' (<b>{point.y}x</b>)'
          },
          data: Object.keys(data).map(function (pid) {
            return {
              name: data[pid].label,
              y: data[pid].count
            }
          })
        };
      }
      else {
        series = [];
        for (var category in user_contributions.categories) {
          if (user_contributions.categories.hasOwnProperty(category)) {
            data = getDataCopy(category, user_contributions.categories).documents;
            series.push({
              name: workflow_categories[category],
              color: workflow_category_colors[category],
              tooltip: {
                pointFormat: '<b>' + username + '</b> - ' + workflow_categories[category] + ' (<b>{point.y}x</b>)'
              },
              data: Object.keys(data).map(function (pid) { return {
                  name: data[pid].label,
                  y: data[pid].count
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

  function arrayCombine(keys, values) {
    var result = [];
    for (var i = 0; i < keys.length; i++) {
      result[keys[i]] = values[i];
    }
    return result;
  }

  Highcharts.Chart.prototype.callbacks.push(function (chart) {
    // This event get triggered after the drilldown, removing the drilling-down
    // class here.
    H.addEvent(chart.container, 'click', function (e) {
      if (chart_container.hasClass('drilling-down')) {
        chart_container.removeClass('drilling-down');
      }
    });
  });
})(jQuery, Drupal, Highcharts);
