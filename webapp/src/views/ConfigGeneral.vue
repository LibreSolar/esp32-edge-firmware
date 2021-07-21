<template>
  <v-container fill-height fluid>
    <v-layout text-center align-center>
      <v-flex>
        <v-card class="mx-auto my-auto" max-width="1200">
          <v-card-title primary-title class="justify-center">Configuration</v-card-title>
          <v-tabs v-if="!loading" v-model="tab" background-color=secodary grow>
            <v-tab
            v-for="node in Object.keys(dataObjects).sort()"
            :key="node">{{ node }}</v-tab>
          </v-tabs>
          <v-tabs-items v-model="tab">
          <v-tab-item
            v-for="[key, values] in Object.entries(dataObjects).sort()"
            :key="key"
          >
            <v-card-text>
            <v-row justify="center" dense no-gutters>
              <v-col
                v-for="name in Object.keys(values)"
                :key="name"
                cols=12
                md="6"
                sm="8"
                lg="6"
                class="px-1 ma-1">
                <v-text-field
                  v-if="!isBoolean(values[name])"
                  :label="name"
                  v-model="values[name]"
                  outlined
                  :dense="$vuetify.breakpoint.smAndDown"
                  class="pa-0 ma-0"
                ></v-text-field>
                <v-checkbox
                  v-else
                  v-model="values[name]"
                  :label="name"></v-checkbox>
              </v-col>
            </v-row>
            <v-btn color="primary" @click="saveValues(key)" class="mx-4">
              <v-icon left>mdi-checkbox-marked-circle</v-icon> Save
            </v-btn>
          </v-card-text>
        </v-tab-item>
        </v-tabs-items>
        <v-btn color="secondary" @click="resetDevice()" class="mx-4">
              <v-icon left>mdi-checkbox-marked-circle</v-icon> Reset Device
            </v-btn>
          <v-card-text>
            <v-alert
              v-model="alert"
              dense
              text
              dismissible
              :type="alert_type"
              transition="scale-transition">
            <v-row align="center">
              <v-col class="grow">{{ status }}</v-col>
              </v-row></v-alert>
          </v-card-text>
        </v-card>
      </v-flex>
    </v-layout>
  </v-container>
</template>

<script>
export default {
  data() {
    return {
      nodes: [],
      dataObjects:{},
      baseData: null,
      diff: {},
      tab: null,
      status: "",
      alert: false,
      alert_type: "warning",
      loading: true,
      timer: null
    }
  },
  mounted() {
    this.fetchAvailableNodes().then(() => {
      this.fetchData().then(() => {
        this.loading = false
      })
    })
  },
  methods: {
    fetchAvailableNodes: function() {
      let id = this.$store.state.self
      return this.$ajax
        .get("api/v1/ts/" + id + "/conf/")
        .then(res => {
          this.alert = false
          this.nodes = res.data
        })
        .catch(error => {
          this.status = "Configuration Information could not be fetched: " + error.response.status + " " + error.response.data
          this.alert_type = "warning"
          this.alert = true
          setTimeout(this.clearAlert, 3000);
        })
    },
    fetchData: function() {
      let promises = []
      let id = this.$store.state.self
      this.nodes.forEach(node => {
        promises.push(this.$ajax
          .get("api/v1/ts/" + id + "/conf/" + node)
          .then(res => {
            this.alert = false
            this.dataObjects[node] = res.data
          })
          .catch(error => {
            this.status = "Configuration Information could not be fetched: " + error.response.status + " " + error.response.data
            this.alert_type = "warning"
            this.alert = true
            setTimeout(this.clearAlert, 3000);
          })
      )})
      return Promise.all(promises)
    },
    saveValues: function(node) {
      let id = this.$store.state.self
      this.$ajax
      .patch("api/v1/ts/" + id + "/conf/" + node, this.dataObjects[node])
      .then(res => {
        this.alert_type = "success"
        this.status = "Values written: Statuscode " + res.status
        this.alert = true
        setTimeout(this.clearAlert, 3000);
      })
      .catch(error => {
        this.status = "Configuration could not be written: " + error.response.status + " " + error.response.data
        this.alert_type = "warning"
        this.alert = true
        setTimeout(this.clearAlert, 3000);
      })
    },
    resetDevice: function() {
      let id = this.$store.state.self
      this.$ajax
      .post("api/v1/ts/" + id + "/exec/reset")
      .then(res => {
        this.alert_type = "success"
        this.status = "Device will restart shortly: Statuscode " + res.status
        this.alert = true
        setTimeout(this.clearAlert, 3000);
      })
      .catch(error => {
        this.status = "Something went wrong: " + error.response.status + " " + error.response.data
        this.alert_type = "warning"
        this.alert = true
        setTimeout(this.clearAlert, 3000);
      })
    },
    isBoolean: function(obj) {
      return (typeof(obj) === 'boolean')
    },
    clearAlert: function() {
      this.alert = false
    }
  }
};
</script>
