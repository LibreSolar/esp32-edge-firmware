<template>
 <v-container fill-height fluid>
    <v-layout text-center align-center>
      <v-flex>
        <v-card class="mx-auto my-auto" max-width="300">
          <v-card-title primary-title class="justify-center">Input/Output</v-card-title>
          <v-card-text v-if="!loading">
            <v-col  dense no-gutters>
                <v-row
                justify="center"
                v-for="(value, name) in dataObjects"
                :key="name"
                cols=12
                md="6"
                sm="8"
                lg="6"
                class="px-1 ma-0">
                <v-checkbox
                    :label="prettyStrings[name] ? prettyStrings[name].title.en : name"
                    v-model="dataObjects[name]"
                    @change="changeValue(name)"
                    outlined
                    :dense="$vuetify.breakpoint.smAndDown"
                    class="pa-0 ma-0"
                ></v-checkbox>
                </v-row>
                <v-row justify="center">
                <v-alert
                v-model="alert"
                dense
                outlined
                type="error"
                transition="scale-transition"
                dismissible
                >{{ status }}</v-alert>
            </v-row>
            </v-col>
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
            dataObjects: null,
            prettyStrings: null,
            loading: true,
            status: "",
            alert: false,
    }
  },
  created() {
    this.fetchData().then(() => {
      this.$store.dispatch('createPrettyStrings', 'input').then((strings) => {
        this.prettyStrings = strings
        this.loading = false
      })
    })
  },
  methods: {
    fetchData: function() {
      let id = this.$store.state.activeDeviceId
      return this.$ajax
        .get("api/v1/ts/" + id + "/input")
        .then(res => {
          this.alert = false
          this.dataObjects = res.data
        })
        .catch(error => {
          this.status = "Information could not be fetched: " + error.response.status + "-" + error.response.data
          this.alert = true
        })
    },

    changeValue: function(name) {
      let id =  this.$store.state.activeDeviceId
      let body = {}
      body[name] = this.dataObjects[name]
      this.$ajax
      .patch("api/v1/ts/" + id + "/input", body)
      .then(res => {
          this.alert = false
      })
      .catch(error => {
        this.status = "Value could not be set: " + error.response.status + "-" + error.response.data
        this.alert = true
      })
    }
  }
};
</script>
