<template>
  <v-container fill-height fluid>
    <v-layout text-center align-center>
      <v-flex>
        <v-card max-width=600 class="mx-auto my-auto">
          <v-img :src="require('../assets/logo.png')" contain height="200"></v-img>
          <v-card-title primary-title>
            <div class="ma-auto">
              Device ID: <span class="grey--text">{{deviceId}}</span>
              <br>
              Hardware type: <span class="grey--text">{{deviceType}}</span>
              <br>
              Hardware version: <span class="grey--text">{{hwVersion}}</span>
              <br>
              Firmware version: <span class="grey--text">{{fwVersion}}</span>
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
      manufacturer: null,
      deviceId: null,
      deviceType: null,
      hwVersion: null,
      fwVersion: null,
      alert: false,
      status: null
    }
  },
  mounted() {
    this.fetchData()
  },
  methods: {
    fetchData: function() {
      let id = this.$store.state.activeDeviceId
      this.$ajax
      .get("api/v1/ts/" + id + "/info")
      .then(res => {
        this.alert = false
        this.manufacturer = res.data.Manufacturer;
        this.deviceId = res.data.DeviceID;
        this.deviceType = res.data.DeviceType;
        this.hwVersion = res.data.HardwareVersion;
        this.fwVersion = res.data.FirmwareVersion;
      })
      .catch(error => {
        this.status = "Device Information could not be fetched: " + error.response.status + "-" + error.response.data
        this.alert = true
      });
    }
  }
}
</script>
