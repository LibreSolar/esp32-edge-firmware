<template>
  <v-container fill-height fluid>
    <v-layout text-center align-center>
      <v-flex class="text-center">
        <v-card max-width="600" class="mx-auto my-auto">
          <v-card-title class="justify-center"
            >Over-the-Air-Upgrade</v-card-title
          >
          <v-card-text>
            <v-row justify="center" dense>
              <v-col cols="10">
                <div class="ma-auto">
                  Select an image from your PC to use. Please note that this process can take up to 5 minutes to complete!
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
                <v-btn
                  :loading="uploading"
                  @click="start()"
                >Start</v-btn>
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
      </v-flex>
    </v-layout>
  </v-container>
</template>

<script>
export default {
  data() {
    return {
      uploading: false,
      file: null,
      status: null,
      alert: false,
      success: false,
      oldFwVersion: null,
      NewFwVersion: null,
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
        let id = this.$store.state.selfInfo.DeviceID;
        this.$ajax
          .post("api/v1/ota/" + id, reader.result)
          .then((res) => {
            this.fetchData("new");
          })
          .catch((error) => {
            this.status =
              error.response.data?.error + " Statuscode: " + error.response.status;
            this.uploading = !this.uploading;
            this.showAlert();
          });
      };
    },
    fetchData: function (target) {
      let id = this.$store.state.selfInfo.DeviceID;
      this.$ajax
        .get("api/v1/ts/" + id + "/info")
        .then((res) => {
          if (target == "old") {
            this.oldFwVersion = res.data.FirmwareVersion;
          } else if (target == "new") {
            this.NewFwVersion = res.data.FirmwareVersion;
            this.status =
              "Updated successfully from " +
              this.oldFwVersion +
              " --> " +
              this.NewFwVersion;
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
  },
};
</script>
