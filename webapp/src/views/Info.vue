<template>
  <v-container fill-height fluid>
    <v-layout text-center align-center>
      <v-flex>
        <v-card max-width=600 class="mx-auto my-auto">
          <v-img :src="require('../assets/logo.png')" contain height="200"></v-img>
          <v-card-title primary-title v-if="!loading">
            <div class="ma-auto">
              Device ID: <span class="grey--text">{{ $store.state.activeDeviceId }}</span>
              <br>
              Manufacturer: <span class="grey--text">{{ $store.state.info[$store.state.activeDeviceId].Manufacturer }}</span>
              <br>
              Hardware type: <span class="grey--text">{{ $store.state.info[$store.state.activeDeviceId].DeviceType }}</span>
              <br>
              Hardware version: <span class="grey--text">{{ $store.state.info[$store.state.activeDeviceId].HardwareVersion }}</span>
              <br>
              Firmware version: <span class="grey--text">{{ $store.state.info[$store.state.activeDeviceId].FirmwareVersion }}</span>
            </div>
          </v-card-title>
          <v-card-text>
            <v-alert
            v-model="alert"
            dense
            text
            type="warning"
            transition="scale-transition"
            ><v-row align="center">
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
      loading: true,
      alert: false,
      status: null
    }
  },
  mounted() {
    if(!this.$store.state.info[this.$store.state.activeDeviceId]) {
      this.$store.dispatch("getDeviceInfo").then(() => {
        this.loading = false;
        })
        .catch(error => {
        this.showError(error)
      })
    } else {
      this.loading = false
    }
  },
  methods: {
    fetchData: function() {
      this.$store.dispatch("getDeviceInfo").then(() => {
        this.alert = false
      })
      .catch(error => {
        this.showError(error)
      });
    },
    showError: function(error) {
      this.status = "Device Information could not be fetched: " + error.response.status + "-" + error.response.data
      this.alert = true
    }
  }
}
</script>
