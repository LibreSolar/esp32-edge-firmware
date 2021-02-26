<template>
  <v-container fill-height>
    <v-layout text-center align-center>
      <v-flex>
        <v-card>
          <v-img :src="require('../assets/logo.png')" contain height="200"></v-img>
          <v-card-title primary-title>
            <div class="ma-auto">
              Device ID: <span class="grey--text">{{device_id}}</span>
              <br>
              Hardware type: <span class="grey--text">{{device_type}}</span>
              <br>
              Hardware version: <span class="grey--text">{{hw_version}}</span>
              <br>
              Firmware version: <span class="grey--text">{{fw_version}}</span>
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
                  <v-btn color="warning" @click="fetch_data()">Reload</v-btn>
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
      device_id: null,
      device_type: null,
      hw_version: null,
      fw_version: null,
      alert: false,
      status: null
    }
  },
  mounted() {
    this.fetch_data()
  },
  methods: {
    fetch_data: function() {
      let id = this.$store.state.active_device_id
      this.$ajax
      .get("api/v1/ts/" + id + "/info")
      .then(res => {
        this.alert = false
        this.manufacturer = res.data.Manufacturer;
        this.device_id = res.data.DeviceID;
        this.device_type = res.data.DeviceType;
        this.hw_version = res.data.HardwareVersion;
        this.fw_version = res.data.FirmwareVersion;
      })
      .catch(error => {
        this.status = "Device Information could not be fetched: " + error.response.status + "-" + error.response.data
        this.alert = true
      });
    }
  }
}
</script>
