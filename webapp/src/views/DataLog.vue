<template>
 <v-container fill-height fluid>
    <v-layout text-center align-center>
      <v-flex>
        <v-card class="mx-auto my-auto" max-width="1200">
          <v-card-title primary-title class="justify-center">Data Log</v-card-title>
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
                    :dense="$vuetify.breakpoint.smAndDown"
                    class="pa-0 ma-0"
                  ></v-text-field>
                </v-col>
            </v-row>
          </v-card-text>
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
  </v-container>
</template>

<script>
export default {
  data() {
    return {
      data: null,
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
      .get("api/v1/ts/" + id + "/rec")
      .then(res => {
        this.alert = false
        this.data = res.data
      })
      .catch(error => {
        this.status = "Recorded Data could not be fetched: " + error.response.status + "-" + error.response.data
        this.alert = true
      })
    }
  }
};
</script>
