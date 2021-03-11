<template>
 <v-container fill-height fluid>
    <v-layout text-center align-center>
      <v-flex>
        <v-card class="mx-auto my-auto" max-width="1200">
          <v-card-title primary-title class="justify-center">Data Log</v-card-title>
          <v-card-text>
            <v-alert
              v-model="alert"
              dense
              text
              :type="alertType"
              dismissible
              transition="scale-transition">{{ status }}
            </v-alert>
          </v-card-text>
          <v-card-text>
            <v-row justify="center" dense no-gutters>
                <v-col
                  v-for="(value, name) in data"
                  :key="name"
                  cols=12
                  xs="8"
                  md="4"
                  sm="6"
                  lg="3"
                  class="px-1 ma-0">
                  <v-text-field
                    :label="$store.state.info ? $store.state.info.rec[name].title.en : name"
                    v-model="data[name]"
                    :suffix="$store.state.info ? $store.state.info.rec[name].unit : (name.search('_') > 0 ? name.split('_')[1].replace('degC', '°C') : '')"
                    outlined
                    readonly
                    append-icon="mdi-delete-outline"
                    @click:append="resetField(name)"
                    :dense="$vuetify.breakpoint.smAndDown"
                    class="pa-0 ma-0"
                  ></v-text-field>
                </v-col>
            </v-row>
          </v-card-text>
        </v-card>
        </v-flex>
        <v-dialog v-model="dialog" width="500" :fullscreen="$vuetify.breakpoint.smAndDown">
      <v-card>
        <v-card-title class="justify-center headline red lighten-2">
          Not Authenticated!
        </v-card-title>
        <v-divider></v-divider>
        <v-card-text>
          To reset values, you need to be authenicated! Please enter the Password given by the Manufacturer:
        </v-card-text>
        <v-card-text>
         <v-text-field
            label="Password"
            v-model="password"></v-text-field>
        </v-card-text>
        <v-divider></v-divider>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn
            color="primary"
            text
            @click="authenticate()">Submit</v-btn>
          <v-spacer></v-spacer>
        </v-card-actions>
      </v-card>
    </v-dialog>
    </v-layout>
  </v-container>
</template>

<script>
export default {
  data() {
    return {
      data: null,
      status: "",
      alert: false,
      alertType: "warning",
      dialog: false,
      password: ""
    }
  },
  created() {
    this.fetchData()
  },
  methods: {
    fetchData: function() {
      let id = this.$store.state.activeDeviceId
      this.$ajax
        .get("api/v1/ts/" + id + "/rec")
        .then(res => {
          this.data = res.data
        })
        .catch(error => {
          this.status = "Recorded Data could not be fetched: " + error.response.status + " " + error.response.data
          this.alertType = "error"
          this.alert = true
        })
    },
    resetField: function(name) {
      this.alert = false;
      if(!this.$store.state.isAuthenticated) {
        this.dialog = true;
        return
      }
    let id = this.$store.state.activeDeviceId
    let payload = {}
    payload[name] = 0
    this.$ajax
      .patch("api/v1/ts/" + id + "/rec", payload)
      .then(res => {
        this.fetchData();
        let label = this.$store.state.info ? this.$store.state.info.rec[name].title.en : name
        this.status = label + " has been reset"
        this.alertType = "success"
        this.alert = true;
      })
      .catch(error => {
        this.status = "Could not be resetted " + error.response.status + " " + error.response.data
        this.alertType = "error"
        this.alert = true
        if(error.response.status == "401") {
          this.dialog = true;
        }
      })
    },
    authenticate: function() {
        this.alert = false;
        this.$store.dispatch('authenticate', this.password)
        .then(res => {
          this.status = "Authentication succesfull"
          this.alertType = "success"
          this.alert = true;
        })
        .catch(error => {
          this.status = "Authentication failed: " + error.response.status + " " + error.response.data
          this.alertType = "warning"
          this.alert = true;
        })
        this.dialog = false;
    }
  }
};
</script>
