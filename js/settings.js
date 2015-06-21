function loadSettings(parent) {
    var settings = coreLib.loadSettings()
    if (settings) {
        settings = JSON.parse(settings)
        for (var i = 0; i < settings.length; i++) {
            var objName = settings[i].name
            for (var j = 0; j < parent.data.length; j++) {
                if (objName === parent.data[j].objectName) {
                    var properties = settings[i].properties
                    for (var prop in properties) {
                        if (parent.data[j].hasOwnProperty(prop)) {
                            var value = properties[prop]
                            if (value === "true") {
                                parent.data[j][prop] = true
                            } else if (value === "false") {
                                parent.data[j][prop] = false
                            } else {
                                parent.data[j][prop] = value.toString()
                            }
                        }
                    }
                    break
                }
            }
        }
        return true
    } else {
        return false
    }
}

function saveSettings(parent) {
    var settings = []
    for (var i = 0; i < parent.data.length; i++) {
        var storage = parent.data[i].storage
        if (storage) {
            var obj = {}
            obj.name = parent.data[i].objectName
            obj.properties = {}
            for (var j = 0; j < storage.length; j++) {
                var prop = storage[j]
                obj.properties[prop] = parent.data[i][prop].toString()
            }
            settings.push(obj)
        }
    }
    coreLib.saveSettings(JSON.stringify(settings, null, 4))
}
