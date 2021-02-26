<template>
 <v-container fill-height fluid>
    <v-layout text-center align-center>
      <v-flex>
        <v-card class="mx-auto my-auto" max-width="1200">
          <v-card-title primary-title class="justify-center">Configuration</v-card-title>
          <v-card-text>
            <v-row justify="center" dense no-gutters>
                <v-col
                  v-for="(value, name) in dataObjects"
                  :key="name"
                  cols=12
                  md="6"
                  sm="8"
                  lg="6"
                  class="px-1 ma-0">
                  <v-text-field
                    :label="$store.state.info ? $store.state.info.conf[name].title.en : name"
                    v-model="dataObjects[name]"
                    :suffix="$store.state.info ? $store.state.info.conf[name].unit : (name.search('_') > 0 ? name.split('_')[1].replace('degC', '°C') : '')"
                    outlined
                    :dense="$vuetify.breakpoint.smAndDown"
                    class="pa-0 ma-0"
                  ></v-text-field>
                </v-col>
            </v-row>
          </v-card-text>
          <v-container class="text-center pa-md-6">
            <v-btn color="primary" @click="sendValues">
              <v-icon left>mdi-checkbox-marked-circle</v-icon> Apply
            </v-btn>
            <v-btn @click="resetValues">
              <v-icon left>mdi-cancel</v-icon> Cancel
            </v-btn>
          </v-container>
          <v-card-text>
            <v-alert
              v-model="alert"
              dense
              text
              type="warning"
              transition="scale-transition">
            <v-row align="center">
              <v-col class="grow">{{ status }}</v-col>
                <v-col class="shrink">
                  <v-btn color="warning" @click="fetchData()">Reload</v-btn>
                </v-col>
              </v-row></v-alert>
          </v-card-text>
        </v-card>
      </v-flex>
    </v-layout>
    <v-dialog v-model="dialog" width="500" :fullscreen="$vuetify.breakpoint.smAndDown">
      <v-card>
        <v-card-title class="headline red lighten-2">
          Something went wrong!
        </v-card-title>
        <v-divider></v-divider>
        <v-card-text>
          This might happen if some of the values are not plausible, the MCU checks new values prior to saving. Please refresh the page to update the values and see what has been written!
        </v-card-text>
        <v-divider></v-divider>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn
            color="primary"
            text
            @click="dialog = false">Ok</v-btn>
          <v-spacer></v-spacer>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </v-container>
</template>

<script>
export default {
  data() {
    return {
      dataObjects: null,
      baseData: null,
      diff: {},
      dialog: false,
      status: "",
      alert: false,
    }
  },
  created() {
    this.fetchData()
  },
  methods: {
    fetchData: function() {
      let id = this.$store.state.activeDeviceId
    this.$ajax
      .get("api/v1/ts/" + id + "/conf")
      .then(res => {
        this.alert = false
        this.dataObjects = res.data
        // keep a copy so we can make a diff to reduce size,
        // writing to eeprom in the MCU takes long...
        // this only works with basic datatypes, not with Date() etc.
        this.baseData = JSON.parse(JSON.stringify(res.data))
      })
      .catch(error => {
        this.status = "Configuration Information could not be fetched: " + error.response.status + "-" + error.response.data
        this.alert = true
      })
    },
    resetValues: function() {
      this.dataObjects = this.baseData
    },
    sendValues: function() {
      let id =  this.$store.state.activeDeviceId
      Object.keys(this.dataObjects).forEach(key => {
        if(this.baseData[key] != this.dataObjects[key]){
          this.diff[key] = this.dataObjects[key]
        }
      })
      this.$ajax
      .patch("api/v1/ts/" + id + "/conf", this.diff)
      .then(res => {
          this.baseData = this.dataObjects
      })
      .catch(error => {
        this.dialog = true
      })
    }
  }
};
</script>
