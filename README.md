# CWRC Credit Visualization
--------------------------------------
Visualize user contributions based on workflow information. Populates a datastore from the Fedora Workflow datastream providing an efficient access method to quick populate visualization tools.

## Maintainers/Sponsors
-----------------------
Current maintainers:

* [CWRC](https://github.com/cwrc)


## License
----------

[GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt)



REQUIREMENTS
------------

  * [Islandora](http://github.com/islandora/islandora)



## Drush scripts to populate datastore from the Fedora Workflow datastream
--------------------------------------------------------------------------
* by PID
* all Fedora Commons content
* all Fedora Commons content modified after a given date

## To automatically populate datastore
--------------------------------------
* isntall https://github.com/cwrc/php_listeners
  * see for details https://github.com/cwrc/php_listeners/blob/master/config.xml.sample
  
## API
------

### Lookup documents by array PID and return all workflow
---

/services/credit_viz?pid[]=x?pid[]=y

### Lookup documents by array collection PIDs and return all workflow
---

/services/credit_viz?collectionId[]=x?collectionId[]=y

### Lookup documents by array user id and return all workflow
---

/services/credit_viz?userId[]=x?userId[]=y

#### URL syntax
services/credit_viz?{params}

#### HTTP Method
GET / POST

#### Get Parameters
| Name            | Description                                                           | Optional            |
| -------------   | --------------------------------------------------------------------- | ------------------- |
| pid[]           | Lookup documents by array of PID and return all workflow              |                     |
| userId[]        | Lookup documents by array of collection PIDs and return all workflow  |                     |
| collectionId[]  | Lookup documents by array of user ids and return all workflow         |                     |

Note: parameter are mutually exclusive, for the moment, and can't be used together.


#### Response: 200 OK
##### Content-Type: application/json
| Name          | Description                                                      |
| ------------- | ---------------------------------------------------------------- |
| workflow      | A Workflow array, with each entry representing a workflow item.

#### Example Response

{
    "query": {"pid": [
        "asdf:asdf"
    ]},
    "documents": [
        {
            "id": "asdf:asdf",
            "name": "Example",
            "modifications": [
                {
                    "user": {
                        "id": "x",
                        "name": "x"
                    },
                    "workflow_changes": {
                        "category": "created",
                        "stamp": "cwrc:cre",
                        "status": "c"
                    },
                    "diff_changes": ["45842"]
                },
                {
                    "user": {
                        "id": "y",
                        "name": "y"
                    },
                    "workflow_changes": {
                        "category": "created",
                        "stamp": "cwrc:cvr",
                        "status": "c"
                    },
                    "diff_changes": ["0"]
                }
            ]
        }
    ]
}
