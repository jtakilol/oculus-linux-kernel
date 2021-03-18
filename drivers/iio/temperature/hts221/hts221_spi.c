/*
 * STMicroelectronics hts221 spi driver
 *
 * Copyright 2016 STMicroelectronics Inc.
 *
 * Lorenzo Bianconi <lorenzo.bianconi@st.com>
 *
 * Licensed under the GPL-2.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/slab.h>
#include "hts221.h"

#define SENSORS_SPI_READ	0x80
#define SPI_AUTO_INCREMENT	0x40

static int hts221_spi_read(struct device *device, u8 addr, int len, u8 *data)
{
	int err;
	struct spi_device *spi = to_spi_device(device);
	struct hts221_dev *dev = spi_get_drvdata(spi);

	struct spi_transfer xfers[] = {
		{
			.tx_buf = dev->tb.tx_buf,
			.bits_per_word = 8,
			.len = 1,
		},
		{
			.rx_buf = dev->tb.rx_buf,
			.bits_per_word = 8,
			.len = len,
		}
	};

	if (len > 1)
		addr |= SPI_AUTO_INCREMENT;
	dev->tb.tx_buf[0] = addr | SENSORS_SPI_READ;

	err = spi_sync_transfer(spi, xfers,  ARRAY_SIZE(xfers));
	if (err < 0)
		return err;

	memcpy(data, dev->tb.rx_buf, len * sizeof(u8));

	return len;
}

static int hts221_spi_write(struct device *device, u8 addr, int len, u8 *data)
{
	struct spi_device *spi = to_spi_device(device);
	struct hts221_dev *dev = spi_get_drvdata(spi);

	struct spi_transfer xfers = {
		.tx_buf = dev->tb.tx_buf,
		.bits_per_word = 8,
		.len = len + 1,
	};

	if (len >= HTS221_TX_MAX_LENGTH)
		return -ENOMEM;

	if (len > 1)
		addr |= SPI_AUTO_INCREMENT;
	dev->tb.tx_buf[0] = addr;
	memcpy(&dev->tb.tx_buf[1], data, len);

	return spi_sync_transfer(spi, &xfers, 1);
}

static const struct hts221_transfer_function hts221_transfer_fn = {
	.read = hts221_spi_read,
	.write = hts221_spi_write,
};

static int hts221_spi_probe(struct spi_device *spi)
{
	int err;
	struct hts221_dev *dev;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	spi_set_drvdata(spi, dev);
	dev->name = spi->modalias;
	dev->dev = &spi->dev;
	dev->irq = spi->irq;
	dev->tf = &hts221_transfer_fn;

	err = hts221_probe(dev);
	if (err < 0) {
		kfree(dev);
		return err;
	}

	dev_info(&spi->dev, "hts221 spi sensor probed\n");

	return 0;
}

static int hts221_spi_remove(struct spi_device *spi)
{
	int err;
	struct hts221_dev *dev = spi_get_drvdata(spi);

	err = hts221_remove(dev);
	if (err < 0)
		return err;

	dev_info(&spi->dev, "hts221 spi sensor removed\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id hts221_spi_of_match[] = {
	{ .compatible = "st,hts221", },
	{},
};
MODULE_DEVICE_TABLE(of, hts221_spi_of_match);
#endif /* CONFIG_OF */

static const struct spi_device_id hts221_spi_id_table[] = {
	{ HTS221_DEV_NAME },
	{},
};
MODULE_DEVICE_TABLE(spi, hts221_spi_id_table);

static struct spi_driver hts221_driver = {
	.driver = {
		.name = "hts221_spi",
#ifdef CONFIG_OF
		.of_match_table = hts221_spi_of_match,
#endif /* CONFIG_OF */
	},
	.probe = hts221_spi_probe,
	.remove = hts221_spi_remove,
	.id_table = hts221_spi_id_table,
};
module_spi_driver(hts221_driver);

MODULE_AUTHOR("Lorenzo Bianconi <lorenzo.bianconi@st.com>");
MODULE_DESCRIPTION("STMicroelectronics hts221 spi driver");
MODULE_LICENSE("GPL v2");
