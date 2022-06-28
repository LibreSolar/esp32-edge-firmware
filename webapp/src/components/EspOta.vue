<template>
  <v-card>
    <v-card-text>
      <v-row justify="center" dense>
        <v-col cols="10">
          <div class="ma-auto">
            Select an image from your PC to use. Please note that this process
            can take up to 5 minutes to complete!
          </div>
        </v-col>
      </v-row>
      <v-row justify="center" dense>
        <v-col cols="10">
          <div class="ma-auto">Current Version: {{ oldFwVersion }}</div>
        </v-col>
      </v-row>
      <v-row justify="center">
        <v-col cols="10">
          <v-file-input
            v-model="file"
            label="File input"
            outlined
            dense
            :show-size="1000"
          ></v-file-input>
        </v-col>
      </v-row>
      <v-row justify="center" dense>
        <v-col cols="2">
          <v-btn :loading="uploading" @click="start()">Start</v-btn>
        </v-col>
      </v-row>
      <v-row justify="center">
        <v-alert
          v-model="alert"
          dense
          outlined
          type="error"
          transition="scale-transition"
          dismissible
          >{{ status }}</v-alert
        >
        <v-alert
          v-model="success"
          dense
          outlined
          type="success"
          transition="scale-transition"
          dismissible
          >{{ status }}</v-alert
        >
      </v-row>
    </v-card-text>
  </v-card>
</template>

<script>
export default {
  props: ["deviceID"],
  data() {
    return {
      uploading: false,
      file: null,
      status: null,
      alert: false,
      success: false,
      oldFwVersion: null,
      newFwVersion: null,
    };
  },
  mounted() {
    this.fetchData("old");
  },
  methods: {
    start: function () {
      this.uploading = !this.uploading;
      var reader = new FileReader();
      if (!this.file) {
        this.status = "No file selected";
        this.showAlert();
        this.uploading = !this.uploading;
        return;
      }
      reader.readAsArrayBuffer(this.file);
      reader.onload = () => {
        this.$ajax
          .post("ota/" + this.deviceID, reader.result)
          .then((res) => {
            this.sleep(5).then(() => {
              this.fetchData("new");
            })
          })
          .catch((error) => {
            this.status =
              error.response.data?.error +
              " Statuscode: " +
              error.response.status;
            this.uploading = !this.uploading;
            this.showAlert();
          });
      };
    },
    fetchData: function (target) {
      this.$ajax
        .get("ts/" + this.deviceID + "/info")
        .then((res) => {
          if (target == "old") {
            this.oldFwVersion = res.data.FirmwareVersion;
          } else if (target == "new") {
            this.newFwVersion = res.data.FirmwareVersion;
            this.status =
              "Updated successfully from " +
              this.oldFwVersion +
              " --> " +
              this.newFwVersion;
            this.showSuccess();
            this.uploading = !this.uploading;
          }
          return;
        })
        .catch((error) => {
          this.status =
            "Device Information could not be fetched: " +
            error.response.status +
            "-" +
            error.response.data;
          this.showAlert();
          this.uploading = !this.uploading;
        });
    },
    showAlert: function () {
      this.success = false;
      this.alert = true;
    },
    showSuccess: function () {
      this.alert = false;
      this.success = true;
    },
    sleep: async function (seconds) {
      return new Promise(resolve => setTimeout(resolve, seconds * 1000));
    }
  },
};
</script>
