<template>
  <v-container>
    <v-layout text-xs-center wrap>
      <v-flex xs12 sm6 offset-sm3>
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
      fw_version: null
    }
  },
  mounted() {
    this.$ajax
      .get('/api/info')
      .then(data => {
        this.manufacturer = data.data.Manufacturer;
        this.device_id = data.data.DeviceID;
        this.device_type = data.data.DeviceType;
        this.hw_version = data.data.HardwareVersion;
        this.fw_version = data.data.FirmwareVersion;
      })
      .catch(error => {
        console.log(error);
      });
  }
}
</script>
